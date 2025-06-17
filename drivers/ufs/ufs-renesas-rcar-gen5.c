// SPDX-License-Identifier: GPL-2.0-only
/*
 * Renesas UFS host controller driver
 *
 * Copyright (C) 2025 Renesas Electronics Corporation
 */

#include <clk.h>
#include <dm.h>
#include <ufs.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/bug.h>
#include <linux/iopoll.h>
#include "ufs.h"

#define CFG_CLK_IGNORE

struct ufs_renesas_priv {
	struct clk_bulk clks;
	bool initialized;	/* The hardware needs initialization once */

	fdt_addr_t phy_base;
};

static void ufs_dme_command(struct ufs_hba *hba, u32 cmd,
			    u32 arg1, u32 arg2, u32 arg3)
{
	ufshcd_writel(hba, arg1, REG_UIC_COMMAND_ARG_1);
	ufshcd_writel(hba, arg2, REG_UIC_COMMAND_ARG_2);
	ufshcd_writel(hba, arg3, REG_UIC_COMMAND_ARG_3);
	ufshcd_writel(hba, cmd, REG_UIC_COMMAND);
}

static int ufs_link_startup_notify(struct ufs_hba *hba,
				   enum ufs_notify_change_status status)
{
	hba->quirks |= UFSHCD_QUIRK_BROKEN_LCC;
	switch (status) {
	case PRE_CHANGE:
		return ufshcd_dme_set(hba, UIC_ARG_MIB(PA_LOCAL_TX_LCC_ENABLE), 0);
	case POST_CHANGE:
	;
	}

	return 0;
}

static int ufs_get_max_pwr_mode(struct ufs_hba *hba,
				struct ufs_pwr_mode_info *max_pwr_info)
{
	max_pwr_info->info.gear_rx = UFS_PWM_G1;
	max_pwr_info->info.gear_tx = UFS_PWM_G1;
	max_pwr_info->info.pwr_tx = SLOWAUTO_MODE;
	max_pwr_info->info.pwr_rx = SLOWAUTO_MODE;
	max_pwr_info->info.hs_rate = 0;

	max_pwr_info->info.lane_rx = 1;
	max_pwr_info->info.lane_tx = 1;

	dev_info(hba->dev, "Max HS Gear: %d\n", max_pwr_info->info.gear_rx);

	return 0;
}

static void ufs_renesas_pre_init(struct ufs_hba *hba)
{
	struct ufs_renesas_priv *priv = dev_get_priv(hba->dev);

	int ret, timeout;
	u16 val;
	u32 val32;

	writew(0x0001, priv->phy_base + 0x20000);	/* 1 */
	writew(0x005c, priv->phy_base + 0x20212);	/* 2 */
	writew(0x005c, priv->phy_base + 0x20214);	/* 3 */
	writew(0x005c, priv->phy_base + 0x20216);	/* 4 */
	writew(0x005c, priv->phy_base + 0x20218);	/* 5 */
	writew(0x036a, priv->phy_base + 0x201d0);	/* 6 */
	writew(0x0102, priv->phy_base + 0x201d2);	/* 7 */
	writew(0x001f, priv->phy_base + 0x20082);	/* 8 */
	writew(0x000b, priv->phy_base + 0x20084);	/* 9 */
	writew(0x0126, priv->phy_base + 0x201d2);	/* 10 */
	writew(0x01dc, priv->phy_base + 0x20214);	/* 12 */
	writew(0x01dc, priv->phy_base + 0x20218);	/* 13 */
	writew(0x0000, priv->phy_base + 0x201cc);	/* 15 */
	writew(0x0200, priv->phy_base + 0x201ce);	/* 16 */
	writew(0x0000, priv->phy_base + 0x20212);	/* 17 */
	writew(0x0000, priv->phy_base + 0x20216);	/* 18 */

	ret = readw_poll_timeout(priv->phy_base + 0x201ec, val, (val & BIT(12)) == 0, 100000);

	if (ret)
		return;

	ret = readw_poll_timeout(priv->phy_base + 0x201e4, val, (val & BIT(12)) == 0, 100000);
	if (ret)
		return;

	ret = readw_poll_timeout(priv->phy_base + 0x201f0, val, (val & BIT(12)) == 0, 100000);

	if (ret)
		return;

	writew(0x0000, priv->phy_base + 0x20000);		/* 19 */

	ufshcd_writel(hba, BIT(0), REG_CONTROLLER_ENABLE);	/* 20 */

	timeout = 100000;

	do {
		val32 = ufshcd_readl(hba, REG_CONTROLLER_ENABLE);
		if (val32 & BIT(0))
			break;
		udelay(1);
	} while (timeout--);

	/* 25 */
	timeout = 100000;
	do {
		val32 = ufshcd_readl(hba, REG_CONTROLLER_STATUS);
		if (val32 & BIT(3))
			break;
		udelay(1);
	} while (timeout--);

	/* 26: Skip IE because we cannot handle interrupts here */
	/* 27 */
	ufs_dme_command(hba, 0x00000002, 0x81010000, 0x00000000, 0x00000005);
	/* 28 */
	ufs_dme_command(hba, 0x00000002, 0x81150000, 0x00000000, 0x00000001);
	/* 29 */
	ufs_dme_command(hba, 0x00000002, 0x81180000, 0x00000000, 0x00000001);
	/* 30 */
	ufs_dme_command(hba, 0x00000002, 0x80090000, 0x00000000, 0x00000000);
	/* 31 */
	ufs_dme_command(hba, 0x00000002, 0x800a0000, 0x00000000, 0x000000c8);
	/* 32 */
	ufs_dme_command(hba, 0x00000002, 0x80090001, 0x00000000, 0x00000000);
	/* 33 */
	ufs_dme_command(hba, 0x00000002, 0x800a0001, 0x00000000, 0x000000c8);
	/* 34 */
	ufs_dme_command(hba, 0x00000002, 0x800a0004, 0x00000000, 0x00000000);
	/* 35 */
	ufs_dme_command(hba, 0x00000002, 0x800b0004, 0x00000000, 0x00000064);
	/* 36 */
	ufs_dme_command(hba, 0x00000002, 0x800a0005, 0x00000000, 0x00000000);
	/* 37 */
	ufs_dme_command(hba, 0x00000002, 0x800b0005, 0x00000000, 0x00000064);
	/* 38 */
	ufs_dme_command(hba, 0x00000002, 0xd0850000, 0x00000000, 0x00000001);

	writew(0x0001, priv->phy_base + 0x20000);	/* 39 */

	/* 40 */
	val = readw(priv->phy_base + 0x20022);

	writew(val & ~BIT(0), priv->phy_base + 0x20022);

	/* 41 */
	ret = readw_poll_timeout(priv->phy_base + (0x00198 << 1), val,
				 (val & BIT(0)) == BIT(0), 100000);
	if (ret)
		return;

	writew(0x0368, priv->phy_base + 0x201d0);	/* 44 */

	/* 45-48 */
	ret = readw_poll_timeout(priv->phy_base + 0x201e4, val, (val & BIT(11)) == 0, 100000);
	if (ret)
		return;
	ret = readw_poll_timeout(priv->phy_base + 0x201e8, val, (val & BIT(11)) == 0, 100000);
	if (ret)
		return;
	ret = readw_poll_timeout(priv->phy_base + 0x201ec, val, (val & BIT(11)) == 0, 100000);
	if (ret)
		return;
	ret = readw_poll_timeout(priv->phy_base + 0x201f0, val, (val & BIT(11)) == 0, 100000);
	if (ret)
		return;

	priv->initialized = true;
}

static int ufs_renesas_hce_enable_notify(struct ufs_hba *hba,
					 enum ufs_notify_change_status status)
{
	struct ufs_renesas_priv *priv = dev_get_priv(hba->dev);

	if (priv->initialized)
		return 0;

	if (status == PRE_CHANGE)
		ufs_renesas_pre_init(hba);

	priv->initialized = true;

	return 0;
}

static int ufs_renesas_init(struct ufs_hba *hba)
{
	struct ufs_renesas_priv *priv = dev_get_priv(hba->dev);

	priv->phy_base = dev_read_addr_name(hba->dev, "phy");

	if (priv->phy_base == FDT_ADDR_T_NONE) {
		printf("Failed to get 'phy' register address\n");
		return 1;
	}

	hba->quirks |= UFSHCD_QUIRK_BROKEN_64BIT_ADDRESS | UFSHCD_QUIRK_HIBERN_FASTAUTO;

	return 0;
}

static struct ufs_hba_ops ufs_renesas_vops = {
	.init		= ufs_renesas_init,
	.hce_enable_notify = ufs_renesas_hce_enable_notify,
	.link_startup_notify	= ufs_link_startup_notify,
	.get_max_pwr_mode = ufs_get_max_pwr_mode,
};

static int ufs_renesas_pltfm_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;

	return ufs_scsi_bind(dev, &scsi_dev);
}

static int ufs_renesas_pltfm_probe(struct udevice *dev)
{
#if !defined(CFG_CLK_IGNORE)
	struct ufs_renesas_priv *priv = dev_get_priv(dev);
#endif
	int err;

#if !defined(CFG_CLK_IGNORE)
	err = clk_get_bulk(dev, &priv->clks);
	if (err < 0)
		return err;

	err = clk_enable_bulk(&priv->clks);
	if (err)
		goto err_clk_enable;
#endif

	err = ufshcd_probe(dev, &ufs_renesas_vops);
	if (err) {
		dev_err(dev, "ufshcd_probe() failed %d\n", err);
		goto err_ufshcd_probe;
	}

	return 0;

err_ufshcd_probe:
#if !defined(CFG_CLK_IGNORE)
	clk_disable_bulk(&priv->clks);
err_clk_enable:
	clk_release_bulk(&priv->clks);
#endif
	return err;
}

static int ufs_renesas_pltfm_remove(struct udevice *dev)
{
	struct ufs_renesas_priv *priv = dev_get_priv(dev);

	clk_disable_bulk(&priv->clks);
	clk_release_bulk(&priv->clks);

	return 0;
}

static const struct udevice_id ufs_renesas_pltfm_ids[] = {
	{ .compatible = "renesas,rcar-gen5-ufs" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(ufs_renesas) = {
	.name		= "ufs-renesas-gen5",
	.id		= UCLASS_UFS,
	.of_match	= ufs_renesas_pltfm_ids,
	.bind		= ufs_renesas_pltfm_bind,
	.probe		= ufs_renesas_pltfm_probe,
	.remove		= ufs_renesas_pltfm_remove,
	.priv_auto	= sizeof(struct ufs_renesas_priv),
};
