// SPDX-License-Identifier: GPL-2.0-only
/*
 * Renesas USB device driver for U-Boot
 *
 * Copyright (C) 2025 Renesas Electronics Corporation
 */

#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <generic-phy.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <reset.h>
#include <usb.h>
#include <usb/xhci.h>
#include "core.h"

#define USB_CTRL_CONF19		0x26
#define USB_CTRL_CONF21		0x2a
#define USB_PHY_CONF1		0x802
#define USB_PHY_CONF13		0x81a
#define USB_PHY_RESSHR_CTRL	0x81c

/* USB_CTRL_CONF19 bit fields */
#define USB_CTRL_CONF19_HNU2P	(0x1 << 0)
#define USB_CTRL_CONF19_HNU3P	(0x0 << 4)
#define USB_CTRL_CONF19_HU2PD	(0x0 << 8)
#define USB_CTRL_CONF19_HU3PD	(0x1 << 9)

#define USB_CTRL_CONF19_CONFIG	(USB_CTRL_CONF19_HNU2P | \
				 USB_CTRL_CONF19_HNU3P | \
				 USB_CTRL_CONF19_HU2PD | \
				 USB_CTRL_CONF19_HU3PD)

#define HIGH_SPEED		0
#define SUPER_SPEED_PLUS	1

struct usb_priv {
	struct udevice		*dev;
	void __iomem		*base;
	struct clk_bulk		clks;
	struct reset_ctl_bulk	resets;
	struct phy		usb3_phy;
	struct udevice		*dwc3;
	enum usb_dr_mode	dr_mode;
	enum usb_device_speed	maximum_speed;
	ofnode			child;
	bool			use_usb3_flow;
	bool			has_usb3;
};

static void usb_configure_registers(struct usb_priv *priv)
{
	writew(0x211, priv->base + USB_CTRL_CONF19);
	dev_info(priv->dev, "USB Controller Configuration Register 19 configured\n");
}

static int init_usb3_phy(struct usb_priv *priv)
{
	int ret;

	if (!priv->has_usb3)
		return 0;

	switch (priv->dr_mode) {
	case USB_DR_MODE_HOST:
		ret = generic_phy_set_mode(&priv->usb3_phy, PHY_MODE_USB_HOST, 0);
		break;
	case USB_DR_MODE_PERIPHERAL:
		ret = generic_phy_set_mode(&priv->usb3_phy, PHY_MODE_USB_DEVICE, 0);
		break;
	case USB_DR_MODE_OTG:
	default:
		ret = generic_phy_set_mode(&priv->usb3_phy, PHY_MODE_USB_OTG, 0);
		break;
	}

	if (ret) {
		dev_err(priv->dev, "Failed to set USB3 PHY mode: %d\n", ret);
		return ret;
	}

	ret = generic_phy_init(&priv->usb3_phy);
	if (ret) {
		dev_err(priv->dev, "Failed to initialize USB3 PHY: %d\n", ret);
		return ret;
	}

	ret = generic_phy_power_on(&priv->usb3_phy);
	if (ret) {
		dev_err(priv->dev, "Failed to power on USB3 PHY: %d\n", ret);
		generic_phy_exit(&priv->usb3_phy);
		return ret;
	}

	dev_info(priv->dev, "USB3 MP-PHY initialized successfully\n");
	return 0;
}

static void rcar_gen5_usb_power_off_ports(struct usb_priv *priv)
{
	void __iomem *xhci_regs;
	fdt_addr_t dwc3_base;
	u32 op_base, reg;
	int port_num, i;

	dwc3_base = ofnode_get_addr(priv->child);
	if (dwc3_base == FDT_ADDR_T_NONE) {
		dev_err(priv->dev, "xhci base reg invalid\n");
		return;
	}

	xhci_regs = map_physmem(dwc3_base, DWC3_XHCI_REGS_END, MAP_NOCACHE);
	if (!xhci_regs) {
		dev_err(priv->dev, "Failed to map xHCI registers\n");
		return;
	}

	op_base = HC_LENGTH(readl(xhci_regs));
	reg = readl(xhci_regs + 0x4);
	port_num = (reg >> 24) & 0x7f;

	for (i = 0; i < port_num; i++) {
		u32 offset = op_base + 0x400 + 0x10 * i;

		reg = readl(xhci_regs + offset);
		reg &= ~BIT(9);
		writel(reg, xhci_regs + offset);
	}

	unmap_physmem(xhci_regs, MAP_NOCACHE);
}

static int rcar_gen5_usb_setup_dwc3(struct usb_priv *priv)
{
	struct udevice *dev = priv->dev;
	int ret;

	if (priv->dr_mode == USB_DR_MODE_HOST || priv->dr_mode == USB_DR_MODE_OTG)
		rcar_gen5_usb_power_off_ports(priv);

	ret = device_find_first_child(dev, &priv->dwc3);
	if (ret || !priv->dwc3) {
		dev_err(dev, "Failed to find DWC3 child device: %d\n", ret);
		return ret ? ret : -ENODEV;
	}

	ret = device_probe(priv->dwc3);
	if (ret) {
		dev_err(dev, "Failed to probe DWC3 child device: %d\n", ret);
		return ret;
	}

	dev_info(dev, "DWC3 child device created successfully\n");
	return 0;
}

static int rcar_gen5_usb_init_usb31_flow(struct usb_priv *priv)
{
	int ret;

	dev_info(priv->dev, "Initializing USB3.1 flow\n");

	ret = init_usb3_phy(priv);
	if (ret) {
		dev_err(priv->dev, "Failed USB3 PHY initialization: %d\n", ret);
		return ret;
	}

	/* Set the USB3.1 PHY to Super-Speed-Plus mode before initializing the DWC3 */
	ret = generic_phy_set_speed(&priv->usb3_phy, SUPER_SPEED_PLUS);
	if (ret) {
		dev_err(priv->dev, "Failed to set TCA register in Super-Speed-Plus: %d\n", ret);
		return ret;
	}

	udelay(20000);
	ret = rcar_gen5_usb_setup_dwc3(priv);
	if (ret)
		return ret;

	dev_info(priv->dev, "USB3.1 SuperSpeed flow completed initialization with MP-PHY\n");
	return ret;
}

static int rcar_gen5_usb_init_usb20_flow(struct usb_priv *priv)
{
	int ret;

	dev_info(priv->dev, "Initializing USB2.0 flow\n");
	if (priv->has_usb3) {
		dev_info(priv->dev, "USB3 controller in USB2 mode (Figure 94.11)\n");
		ret = init_usb3_phy(priv);
		if (ret) {
			dev_err(priv->dev, "Failed MP-PHY initialization: %d\n", ret);
			return ret;
		}
		usb_configure_registers(priv);
		ret = generic_phy_set_speed(&priv->usb3_phy, HIGH_SPEED);
		if (ret) {
			dev_err(priv->dev, "Failed to set TCA register in High-Speed: %d\n", ret);
			return ret;
		}
	} else {
		dev_info(priv->dev, "Native USB2 controller (Figure 94.12)\n");
		usb_configure_registers(priv);
	}

	/*
	 * The datasheet describes initialization procedure without full
	 * information about the registers. Therefore, the source code is
	 * based on the bare metal code shared by the board team.
	 */
	writew(0x00000011, priv->base + USB_PHY_RESSHR_CTRL);
	writew(0x00000000, priv->base + USB_PHY_CONF13);
	writew(0x00000001, priv->base + USB_PHY_CONF1);
	udelay(20000);
	writew(0x00000000, priv->base + USB_PHY_CONF1);
	writew(0x00000001, priv->base + USB_CTRL_CONF21);
	writew(0x00000001, priv->base + USB_PHY_CONF13);
	udelay(20000);

	ret = rcar_gen5_usb_setup_dwc3(priv);
	if (ret)
		return ret;

	dev_info(priv->dev, "USB2.0 flow completed\n");
	return 0;
}

static int rcar_gen5_usb_init_hardware(struct usb_priv *priv)
{
	int ret;

	if (priv->use_usb3_flow) {
		ret = rcar_gen5_usb_init_usb31_flow(priv);
		if (ret) {
			dev_err(priv->dev, "Failed to initialize USB3.1 flow: %d\n", ret);
			return ret;
		}
	} else {
		ret = rcar_gen5_usb_init_usb20_flow(priv);
		if (ret) {
			dev_err(priv->dev, "Failed to initialize USB2.0 flow: %d\n", ret);
			return ret;
		}
	}

	return ret;
}

static int rcar_gen5_usb_bind(struct udevice *parent)
{
	ofnode node;
	int ret;

	ofnode_for_each_subnode(node, dev_ofnode(parent)) {
		const char *name = ofnode_get_name(node);
		const char *driver;
		enum usb_dr_mode dr_mode;

		dr_mode = usb_get_dr_mode(node);
		if (dr_mode == USB_DR_MODE_UNKNOWN)
			dr_mode = usb_get_dr_mode(dev_ofnode(parent));

		if (CONFIG_IS_ENABLED(DM_USB_GADGET) &&
		    (dr_mode == USB_DR_MODE_PERIPHERAL || dr_mode == USB_DR_MODE_OTG)) {
			driver = "dwc3-generic-peripheral";
		} else if (CONFIG_IS_ENABLED(USB_HOST) && dr_mode == USB_DR_MODE_HOST) {
			driver = "dwc3-generic-host";
		} else {
			continue;
		}

		ret = device_bind_driver_to_node(parent, driver, name, node, NULL);
		if (ret) {
			printf("R-Car Gen5 USB: Failed to bind %s to node %s\n",
			       driver, name);
			return ret;
		}
	}

	return 0;
}

static int rcar_gen5_usb_probe(struct udevice *dev)
{
	struct usb_priv *priv = dev_get_priv(dev);
	ofnode node;
	int ret;

	priv->dev = dev;

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base) {
		dev_err(dev, "Failed to get base address\n");
		return -EINVAL;
	}

	ret = reset_get_bulk(dev, &priv->resets);
	if (ret) {
		dev_err(dev, "Failed to get reset control: %d\n", ret);
		return ret;
	}

	ret = reset_assert_bulk(&priv->resets);
	if (ret) {
		dev_err(dev, "Failed to assert resets: %d\n", ret);
		goto err_reset;
	}

	ret = reset_deassert_bulk(&priv->resets);
	if (ret) {
		dev_err(dev, "Failed to deassert resets: %d\n", ret);
		goto err_reset;
	}

	ret = clk_get_bulk(dev, &priv->clks);
	if (ret) {
		dev_err(dev, "Failed to get clocks: %d\n", ret);
		goto err_reset;
	}

	ret = clk_enable_bulk(&priv->clks);
	if (ret) {
		dev_err(dev, "Failed to enable clocks: %d\n", ret);
		goto err_clk;
	}

	priv->has_usb3 = (generic_phy_get_by_name(dev, "usb3-phy",
						   &priv->usb3_phy) == 0);

	if (!priv->has_usb3)
		dev_info(dev, "Operating in USB2.0 mode (no USB3 MP-PHY)\n");

	priv->child = ofnode_null();
	ofnode_for_each_subnode(node, dev_ofnode(dev)) {
		if (ofnode_device_is_compatible(node, "synopsys,dwc3")) {
			priv->child = node;
			break;
		}
	}

	if (!ofnode_valid(priv->child)) {
		dev_err(dev, "Failed to find DWC3 child node\n");
		ret = -ENODEV;
		goto err_clk;
	}

	priv->dr_mode = usb_get_dr_mode(priv->child);
	if (priv->dr_mode == USB_DR_MODE_UNKNOWN)
		priv->dr_mode = usb_get_dr_mode(dev_ofnode(dev));

	priv->maximum_speed = usb_get_maximum_speed(priv->child);
	switch (priv->maximum_speed) {
	case USB_SPEED_SUPER_PLUS:
	case USB_SPEED_SUPER:
		priv->use_usb3_flow = true;
		break;
	case USB_SPEED_HIGH:
		priv->use_usb3_flow = false;
		break;
	case USB_SPEED_FULL:
	case USB_SPEED_LOW:
		priv->use_usb3_flow = false;
		break;
	case USB_SPEED_UNKNOWN:
	default:
		priv->use_usb3_flow = priv->has_usb3;
		priv->maximum_speed = USB_SPEED_SUPER_PLUS;
		break;
	}

	ret = rcar_gen5_usb_init_hardware(priv);
	if (ret) {
		dev_err(dev, "Failed to initialize hardware: %d\n", ret);
		goto err_hw;
	}

	dev_info(dev, "Renesas USB probed successfully\n");
	return 0;

err_hw:
	if (priv->has_usb3) {
		generic_phy_power_off(&priv->usb3_phy);
		generic_phy_exit(&priv->usb3_phy);
	}
err_clk:
	clk_disable_bulk(&priv->clks);
	clk_release_bulk(&priv->clks);
err_reset:
	reset_assert_bulk(&priv->resets);
	reset_release_bulk(&priv->resets);
	return ret;
}

static int rcar_gen5_usb_remove(struct udevice *dev)
{
	struct usb_priv *priv = dev_get_priv(dev);

	if (priv->dwc3)
		device_remove(priv->dwc3, DM_REMOVE_NORMAL);

	if (priv->has_usb3) {
		generic_phy_power_off(&priv->usb3_phy);
		generic_phy_exit(&priv->usb3_phy);
	}

	clk_disable_bulk(&priv->clks);
	clk_release_bulk(&priv->clks);
	reset_assert_bulk(&priv->resets);
	reset_release_bulk(&priv->resets);

	dev_info(dev, "Renesas USB3 glue layer removed\n");
	return 0;
}

static const struct udevice_id rcar_gen5_usb_of_match[] = {
	{ .compatible = "renesas,rcar-gen5-usb" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(renesas_rcar_gen5_usb) = {
	.name = "renesas_rcar_gen5_usb",
	.id = UCLASS_NOP,
	.of_match = rcar_gen5_usb_of_match,
	.bind = rcar_gen5_usb_bind,
	.probe = rcar_gen5_usb_probe,
	.remove = rcar_gen5_usb_remove,
	.priv_auto = sizeof(struct usb_priv),
	.flags = DM_FLAG_OS_PREPARE,
};
