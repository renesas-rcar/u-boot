// SPDX-License-Identifier: GPL-2.0-only
/* Renesas Multi-Protocol PHY device driver
 *
 * Copyright (C) 2025 Renesas Electronics Corporation
 */

#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <generic-phy.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <reset.h>
#include <linux/iopoll.h>
#include <command.h>
#include <env.h>
#include "mp_phy_fw_obj.h"

/* Common registers */
#define MPPHY_CMNCNT1			0x80000
#define MPPHY_CMNCNT2			0x80004
#define MPPHY_PCS0REG1			0x85000
#define MPPHY_PCS0REG5			0x85010

/* Channel register base and offsets */
#define MPPHY_CHAN_BASE(ch)		(0x81000 + (ch) * 0x1000)
#define MPPHY_PXTEST_OFFSET		0x00C
#define MPPHY_RXCNT_OFFSET		0x038
#define MPPHY_SRAMCNT_OFFSET		0x040
#define MPPHY_REFCLK_OFFSET		0x014
#define MPPHY_CNTXT1_OFFSET		0x004
#define MPPHY_CNTXT2_OFFSET		0x008
#define MPPHY_TXREQ_OFFSET		0x044
#define MPPHY_RXREQ1_OFFSET		0x024

/* Channel specific registers */
#define MPPHY_PXTEST(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_PXTEST_OFFSET)
#define MPPHY_PXRXCNT(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_RXCNT_OFFSET)
#define MPPHY_PXSRAMCNT(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_SRAMCNT_OFFSET)
#define MPPHY_PXREFCLK(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_REFCLK_OFFSET)
#define MPPHY_PXCNTXT1(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_CNTXT1_OFFSET)
#define MPPHY_PXCNTXT2(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_CNTXT2_OFFSET)
#define MPPHY_PXTXREQ(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_TXREQ_OFFSET)
#define MPPHY_PXRXREQ1(ch)		(MPPHY_CHAN_BASE(ch) + MPPHY_RXREQ1_OFFSET)

/* Channel enable bit masks for MPPHY_CMNCNT1 register */
#define MPPHY_CMNCNT1_CH_MASK(ch)	(0xFF << ((ch) * 8))

/* Channel enable bits for MPPHY_CMNCNT1 register */
#define MPPHY_CMNCNT1_ETH_EN(ch)	(0x01 << ((ch) * 8))
#define MPPHY_CMNCNT1_USB_EN(ch)	((ch) == 0 ? -1 : \
					 (ch) == 1 ? -1 : 0x03 << ((ch) * 8))

/* PCS0REG5 register mask and values for each channel */
#define MPPHY_PCS0REG5_CH(ch)		(0x03 << (24 + (ch) * 2))

/* PCS0REG1 register bits */
#define MPPHY_PCS0REG1_VAL		0x00010000

/* PXTEST register bit */
#define MPPHY_PXTEST_BIT		BIT(0)

/* PXRXCNT register reset value */
#define MPPHY_PXRXCNT_RESET_VAL		0x202

/* PXSRAMCNT register bits */
#define MPPHY_PXSRAMCNT_BYPASS		BIT(0)
#define MPPHY_PXSRAMCNT_BIT3		BIT(3)
#define BOOTLOAD_BYPASS_MODE		0x3
#define SRAM_BYPASS_MODE		0xc
#define SRAM_EXT_LD_DONE		0x10
#define SRAM_INIT_DONE			0x20

#define SRAM_CONTROL_SET_BIT		\
	(BOOTLOAD_BYPASS_MODE | SRAM_BYPASS_MODE | \
	 SRAM_EXT_LD_DONE | SRAM_INIT_DONE)

/* CMNCNT1/2 clock settings */
#define MPPHY_CMNCNT2_CLK_CH(ch)	(0x30003 << ((ch) * 4))

/* PXREFCLK register value */
#define MPPHY_PXREFCLK_VAL		0x35

/* PXTXREQ register value */
#define MPPHY_PXTXREQ_VAL		0x8

/* Context settings */
#define MPPHY_CNTXT1_VALUE		0x02010002
#define MPPHY_CNTXT2_VALUE		0x02020202 /* For channels 1-3 */
#define MPPHY_CNTXT2_CH0_VALUE		0x02020201 /* Special for channel 0 */

#define MPPHY_NUM_CHANNELS		4

/*-------------------------------------- TCA define -------------------------------*/
/* TCA (Type-C Adapter) Register Offsets within MP-PHY base */
#define MPPHY_USB_BASE(ch)			(0x90000 + (ch) * 0x10000)

/* Channel specific registers */
#define TCA_INTR_OFFSET(ch)			(MPPHY_USB_BASE(ch) + 0x4)
#define TCA_INTR_STS_OFFSET(ch)			(MPPHY_USB_BASE(ch) + 0x8)
#define TCA_TCPC_OFFSET(ch)			(MPPHY_USB_BASE(ch) + 0x14)
#define TCA_VBUS_CTRL_OFFSET(ch)		(MPPHY_USB_BASE(ch) + 0x0040)
#define PSTATE_1_OFFSET(ch)			(MPPHY_USB_BASE(ch) + 0x0054)
/*---------------------------------------------------------------------------------*/

#define HIGH_SPEED		0
#define SUPER_SPEED_PLUS	1

/* Firmware update */
#define MPPHY_FW_BASE		0x10000
#define MPPHY_FW_CH_OFFSET	0x20000

#define is_usb_mode(mode) ((mode) == PHY_MODE_USB_HOST || \
			 (mode) == PHY_MODE_USB_DEVICE || \
			 (mode) == PHY_MODE_USB_OTG)

struct mp_phy_chan_priv {
	unsigned int channel_id;
	unsigned int lane_id;
	unsigned int protocol_id;
	bool initialized;
	enum phy_mode current_protocol;
	int speed;
};

/* struct mpphy_priv - Private data for the MPPHY driver */
struct mp_phy_priv {
	struct phy		*phy;
	struct clk_bulk		clks;
	struct reset_ctl_bulk	resets;
	void __iomem		*base;
	struct mp_phy_chan_priv chan[MPPHY_NUM_CHANNELS];
};

static void mp_phy_write(void __iomem *base, u32 offset, u32 value)
{
	writel(value, base + offset);
}

static void mp_phy_update_bits(void __iomem *base, u32 offset, u32 mask, u32 value)
{
	u32 tmp;

	tmp = readl(base + offset);
	tmp = (tmp & ~mask) | (value & mask);
	writel(tmp, base + offset);
}

static int mp_phy_reg_wait(void __iomem *base, u32 offs, u32 mask, u32 expected)
{
	u32 val;

	if (!base) {
		pr_err("mpphy_reg_wait: Invalid address\n");
		return -EINVAL;
	}

	int ret = readl_poll_sleep_timeout(base + offs, val, (val & mask) == expected,
										1, 10000000);

	if (ret) {
		pr_err("%s: Timeout waiting addr: 0x%p, offset: 0x%x, mask: 0x%x, exp: 0x%x\n",
		       __func__, base, offs, mask, expected);
	} else {
		pr_debug("%s: Success addr: 0x%p, offset: 0x%x, val: 0x%x\n",
			 __func__, base, offs, val);
	}

	return ret;
}

static int mp_phy_update_firmware(struct mp_phy_priv *priv, u32 channel_id)
{
	u32 offset;
	int i;

	/* Checked at build time: is the filename a non-empty? */
	if (sizeof(CONFIG_PHY_R8A78000_MP_PHY_FW) <= 1) {
		pr_err("MP-PHY firmware not linked into this build ");
		pr_err("(set CONFIG_PHY_R8A78000_MP_PHY_FW)\n");
		return -EINVAL;
	}

	pr_info("Loading MP-PHY firmware: %d bytes to channel %d\n",
		mp_phy_fw_obj_size, channel_id);
	offset = MPPHY_FW_BASE + MPPHY_FW_CH_OFFSET * channel_id;
	for (i = 0; i < ((mp_phy_fw_obj_size + 1) / 2); i++)
		writew(((const u16 *)mp_phy_fw_obj)[i], priv->base + offset + (i * 2));

	return 0;
}

static int mp_phy_init_ethernet(struct mp_phy_priv *priv, u32 channel_id)
{
	mp_phy_update_bits(priv->base, MPPHY_CMNCNT1, MPPHY_CMNCNT1_CH_MASK(channel_id),
			   MPPHY_CMNCNT1_ETH_EN(channel_id));

	mp_phy_update_bits(priv->base, MPPHY_PCS0REG5,
			   MPPHY_PCS0REG5_CH(channel_id), MPPHY_PCS0REG5_CH(channel_id));
	mp_phy_update_bits(priv->base, MPPHY_PCS0REG1, MPPHY_PCS0REG1_VAL, MPPHY_PCS0REG1_VAL);
	mp_phy_update_bits(priv->base, MPPHY_PXTEST(channel_id),
			   MPPHY_PXTEST_BIT, MPPHY_PXTEST_BIT);

	mp_phy_update_bits(priv->base, MPPHY_PCS0REG5, MPPHY_PCS0REG5_CH(channel_id), 0x00);
	mp_phy_update_bits(priv->base, MPPHY_PCS0REG1, MPPHY_PCS0REG1_VAL, 0x00);
	mp_phy_update_bits(priv->base, MPPHY_PXTEST(channel_id), MPPHY_PXTEST_BIT, 0x00);

	mp_phy_write(priv->base, MPPHY_PXRXCNT(channel_id), MPPHY_PXRXCNT_RESET_VAL);
	mp_phy_update_bits(priv->base, MPPHY_PXSRAMCNT(channel_id),
			   MPPHY_PXSRAMCNT_BYPASS, MPPHY_PXSRAMCNT_BYPASS);
	mp_phy_update_bits(priv->base, MPPHY_PXSRAMCNT(channel_id),
			   MPPHY_PXSRAMCNT_BIT3, MPPHY_PXSRAMCNT_BIT3);
	mp_phy_update_bits(priv->base, MPPHY_CMNCNT2,
			   MPPHY_CMNCNT2_CLK_CH(channel_id), MPPHY_CMNCNT2_CLK_CH(channel_id));
	mp_phy_update_bits(priv->base, MPPHY_PXREFCLK(channel_id),
			   MPPHY_PXREFCLK_VAL, MPPHY_PXREFCLK_VAL);

	mp_phy_write(priv->base, MPPHY_PXRXCNT(channel_id), 0x0);
	mp_phy_write(priv->base, MPPHY_PXCNTXT1(channel_id), MPPHY_CNTXT1_VALUE);
	mp_phy_write(priv->base, MPPHY_PXCNTXT2(channel_id),
		     (channel_id == 0) ? MPPHY_CNTXT2_CH0_VALUE : MPPHY_CNTXT2_VALUE);
	mp_phy_write(priv->base, MPPHY_PXTXREQ(channel_id), MPPHY_PXTXREQ_VAL);

	pr_info("Ethernet PHY Channel %d Initialization done\n", channel_id);

	return 0;
}

static int mp_phy_init_usb(struct mp_phy_priv *priv, u32 channel_id)
{
	u32 data;
	u32 sramcnt = 0x09;
	u32 cmncnt2 = 0x33001100;

	mp_phy_update_bits(priv->base, MPPHY_CMNCNT1,
			   MPPHY_CMNCNT1_USB_EN(channel_id), MPPHY_CMNCNT1_USB_EN(channel_id));
	mp_phy_update_bits(priv->base, MPPHY_CMNCNT2, cmncnt2, cmncnt2);

	mp_phy_update_bits(priv->base, MPPHY_PXTEST(channel_id),
			   MPPHY_PXTEST_BIT, MPPHY_PXTEST_BIT);
	mp_phy_write(priv->base, MPPHY_PXSRAMCNT(channel_id), sramcnt);
	mp_phy_update_bits(priv->base, MPPHY_PXTEST(channel_id), MPPHY_PXTEST_BIT, 0x0);

	mp_phy_update_bits(priv->base, MPPHY_PCS0REG1, MPPHY_PCS0REG1_VAL, 0x0);
	mp_phy_update_bits(priv->base, MPPHY_PCS0REG5, 0xff000000, 0x0);

	data = mp_phy_reg_wait(priv->base, MPPHY_PXSRAMCNT(channel_id), 0x00000020, 0x00000020);
	if (data) {
		pr_err("Timeout waiting for MPPHY_PXSRAMCNT(%d) configuration\n", channel_id);
		return data;
	}

	mp_phy_update_firmware(priv, channel_id);

	mp_phy_update_bits(priv->base, MPPHY_PXSRAMCNT(channel_id), SRAM_EXT_LD_DONE,
			   SRAM_EXT_LD_DONE);
	data = mp_phy_reg_wait(priv->base, MPPHY_PXRXREQ1(channel_id), 0x00000002, 0x00000000);
	if (data) {
		pr_err("Timeout waiting for MPPHY_PXRXREQ1(%d) configuration\n", channel_id);
		return data;
	}

	return 0;
}

static int mp_phy_init(struct phy *phy)
{
	int ret = 0;
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);
	struct mp_phy_chan_priv *chan = &priv->chan[phy->id];
	u32 channel_id = phy->id;
	enum phy_mode protocol = chan->protocol_id;

	if (phy->id > 3) {
		pr_err("Invalid channel ID: %ld\n", phy->id);
		return -EINVAL;
	}

	/* Auto-detect protocol if not set, based on channel ID */
	if (protocol == 0) {
		if (channel_id == 2) {
			pr_warn("WARNING: protocol is 0. Auto-forcing PHY_MODE_ETHERNET for channel 2\n");
			protocol = PHY_MODE_ETHERNET;
			chan->protocol_id = PHY_MODE_ETHERNET;
		} else if (channel_id == 3) {
			pr_warn("WARNING: protocol is 0. Auto-forcing PHY_MODE_USB_HOST for channel 3\n");
			protocol = PHY_MODE_USB_HOST;
			chan->protocol_id = PHY_MODE_USB_HOST;
		}
	}

	/* Check if initialized with same protocol then skip */
	if (chan->initialized && chan->current_protocol == protocol) {
		pr_info("PHY on channel %d already initialized with protocol %d\n",
			channel_id, protocol);
		return 0;
	}

	/* If already initialized with a different protocol, return error */
	switch (protocol) {
	case PHY_MODE_ETHERNET:
		ret = mp_phy_init_ethernet(priv, channel_id);
		break;
	case PHY_MODE_USB_HOST:
	case PHY_MODE_USB_DEVICE:
	case PHY_MODE_USB_OTG:
		ret = mp_phy_init_usb(priv, channel_id);
		break;
	default:
		pr_err("Unsupported protocol: %d\n", protocol);
		return -EOPNOTSUPP;
	}

	if (!ret) {
		chan->initialized = true;
		chan->current_protocol = protocol;
	}

	return 0;
}

static int mp_phy_exit(struct phy *phy)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);
	struct mp_phy_chan_priv *chan = &priv->chan[phy->id];

	if (!chan->initialized)
		return 0;

	chan->initialized = false;
	chan->current_protocol = PHY_MODE_INVALID;

	return 0;
}

static int mp_phy_late_init(struct phy *phy)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);
	struct mp_phy_chan_priv *chan = &priv->chan[phy->id];

	if (chan->protocol_id == PHY_MODE_ETHERNET)
		writel(SRAM_CONTROL_SET_BIT, priv->base + MPPHY_PXSRAMCNT(phy->id));
	return 0;
}

static int mp_phy_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);
	struct mp_phy_chan_priv *chan = &priv->chan[phy->id];

	/* Only support Ethernet and USB modes */
	if (mode != PHY_MODE_ETHERNET && !is_usb_mode(mode))
		return -EOPNOTSUPP;

	/* If already initialized with the same protocol, no need to do anything */
	if (chan->initialized && chan->current_protocol == mode)
		return 0;

	if (chan->initialized) {
		/* If currently in USB mode and switching to another mode */
		if (!(is_usb_mode(chan->current_protocol) && is_usb_mode(mode)))
			return -EINVAL;
	}

	chan->current_protocol = mode;
	chan->protocol_id = mode;
	return 0;
}

static int mp_phy_config_usb(struct phy *phy, int speed)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);
	struct mp_phy_chan_priv *chan = &priv->chan[phy->id];
	u32 data;

	pr_debug("Configuring USB PHY on channel %ld with %s\n", phy->id,
		 (speed == HIGH_SPEED) ? "High Speed" : "Super Speed Plus");

	switch (speed) {
	case HIGH_SPEED:
		mp_phy_write(priv->base, TCA_VBUS_CTRL_OFFSET(chan->lane_id), 0x0000003E);
		break;
	case SUPER_SPEED_PLUS:
		data = mp_phy_reg_wait(priv->base, PSTATE_1_OFFSET(chan->lane_id),
				       0x00000003, 0x00000003);
		if (data) {
			pr_err("Timeout waiting for PSTATE_1_OFFSET(%d)\n",
			       chan->lane_id);
			return data;
		}

		mp_phy_update_bits(priv->base, TCA_INTR_OFFSET(chan->lane_id), 0x3, 0x3);

		mp_phy_update_bits(priv->base, TCA_TCPC_OFFSET(chan->lane_id), 0x10, 0x10);
		data = mp_phy_reg_wait(priv->base, TCA_INTR_STS_OFFSET(chan->lane_id),
				       0x00000001, 0x00000001);
		if (data) {
			pr_err("Timeout waiting for TCA_INTR_STS_OFFSET(%d)\n", chan->lane_id);
			return data;
		}

		mp_phy_update_bits(priv->base, TCA_INTR_STS_OFFSET(chan->lane_id), 0x1503, 0x1503);
		mp_phy_update_bits(priv->base, TCA_TCPC_OFFSET(chan->lane_id), 0x11, 0x11);
		data = mp_phy_reg_wait(priv->base, TCA_INTR_STS_OFFSET(chan->lane_id),
				       0x00000001, 0x00000001);
		if (data) {
			pr_err("Timeout waiting for TCA_INTR_STS_OFFSET(%d)\n", chan->lane_id);
			return data;
		}
		mp_phy_update_bits(priv->base, TCA_INTR_STS_OFFSET(chan->lane_id), 0x1503, 0x1503);
		break;
	}
	return 0;
}

static int mp_phy_of_xlate(struct phy *phy, struct ofnode_phandle_args *args)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);

	if (args->args_count > 2) {
		pr_err("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	if (args->args_count >= 1)
		phy->id = args->args[0];
	else
		phy->id = 0;

	if (args->args_count >= 2)
		priv->chan[phy->id].lane_id = args->args[1];
	else
		priv->chan[phy->id].lane_id = 0;

	return 0;
}

static const struct phy_ops mp_phy_ops = {
	.init		= mp_phy_init,
	.exit		= mp_phy_exit,
	.power_on	= mp_phy_late_init,
	.set_mode	= mp_phy_set_mode,
	.set_speed	= mp_phy_config_usb,
	.of_xlate	= mp_phy_of_xlate,
};

static int mp_phy_probe(struct udevice *dev)
{
	struct mp_phy_priv *priv = dev_get_priv(dev);
	int ret, i;

	/* Get base address from device tree */
	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	ret = clk_get_bulk(dev, &priv->clks);
	if (ret < 0)
		return ret;

	ret = clk_enable_bulk(&priv->clks);
	if (ret)
		goto err_clk_enable;

	ret = reset_get_bulk(dev, &priv->resets);
	if (ret)
		goto err_reset_get;

	ret = reset_assert_bulk(&priv->resets);
	if (ret)
		goto err_reset_assert;

	ret = reset_deassert_bulk(&priv->resets);
	if (ret)
		goto err_reset_assert;

	for (i = 0; i < MPPHY_NUM_CHANNELS; i++) {
		priv->chan[i].initialized = false;
		priv->chan[i].current_protocol = PHY_MODE_INVALID;
		priv->chan[i].protocol_id = PHY_MODE_INVALID;
	}

	return 0;

err_reset_assert:
	reset_release_bulk(&priv->resets);
err_reset_get:
	clk_disable_bulk(&priv->clks);
err_clk_enable:
	clk_release_bulk(&priv->clks);
	return ret;
}

static const struct udevice_id mp_phy_ids[] = {
	{ .compatible = "renesas,r8a78000-multi-protocol-phy" },
	{ }
};

U_BOOT_DRIVER(renesas_mpphy) = {
	.name		= "renesas_mpphy",
	.id		= UCLASS_PHY,
	.of_match	= mp_phy_ids,
	.probe		= mp_phy_probe,
	.ops		= &mp_phy_ops,
	.priv_auto	= sizeof(struct mp_phy_priv),
};
