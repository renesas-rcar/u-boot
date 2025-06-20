// SPDX-License-Identifier: GPL-2.0-only
/* Renesas Multi-Protocol PHY device driver
 *
 * Copyright (C) 2025 Renesas Electronics Corporation
 */

 #include <asm/io.h>
 #include <clk-uclass.h>
 #include <clk.h>
 #include <div64.h>
 #include <dm.h>
 #include <dm/device_compat.h>
 #include <dm/lists.h>
 #include <dm/of_access.h>
 #include <generic-phy.h>
 #include <linux/bitfield.h>
 #include <linux/bitops.h>
 #include <linux/delay.h>
 #include <linux/iopoll.h>
 #include <log.h>
 #include <reset.h>
 #include <syscon.h>

 /* Hardcoded for enable module clock */
#define MDLC_BASE		0xc9c90000
#define MPPHY_PDID		(0)
#define MPPHY_CLK_MASK(n)	GENMASK((n) + 1, n)
#define MPPHY_CLK_SHIFT(n)	(n)

#define MDLC_PKCPROT0		(MDLC_BASE + 0x0cf0)
#define MDLC_PKCPROT1		(MDLC_BASE + 0x0cf4)

#define _MDLC_MPDG(k)		(MDLC_BASE + 0x0200 + (k) * 4)
#define _MDLC_MPDGS(k)		(MDLC_BASE + 0x0300 + (k) * 4)
#define MDLC_MPIER0		(MDLC_BASE + 0x0110)
#define MDLC_MPIMR0		(MDLC_BASE + 0x0120)

#define MDLC_MPDG		_MDLC_MPDG(MPPHY_PDID)
#define MDLC_MPDGS		_MDLC_MPDGS(MPPHY_PDID)

#define MDLC_MSRES(i)		(MDLC_BASE + 0x0900 + (i) * 4)
#define MDLC_MSRESS(i)	(	MDLC_BASE + 0x0960 + (i) * 4)

#if CONFIG_IS_ENABLED(RCAR_SCP_FIXUP)
static void mp_phy_module_power_gating_set(u8 pdid, u8 mode)
{
	void __iomem *unlock = map_physmem(MDLC_PKCPROT0, 4, MAP_NOCACHE);
	void __iomem *mpdg = map_physmem(_MDLC_MPDG(pdid), 4, MAP_NOCACHE);
	void __iomem *mpdgs = map_physmem(_MDLC_MPDGS(pdid), 4, MAP_NOCACHE);
	void __iomem *mpier0 = map_physmem(MDLC_MPIER0, 4, MAP_NOCACHE);
	void __iomem *mpimr0 = map_physmem(MDLC_MPIMR0, 4, MAP_NOCACHE);

	writel(0xA5A5A501, unlock);

	if ((readl(mpdgs) & 0x3) == mode)
		goto unmap;

	while (readl(mpdgs) != readl(mpdg))
		udelay(1000);

	writel(0, mpier0);
	writel(0x1, mpimr0);

	writel(0x1, mpdg);

	while (readl(mpdgs) != readl(mpdg))
		udelay(1000);

	writel(mode, mpdg);

	while (readl(mpdgs) != readl(mpdg))
		udelay(1000);

unmap:
	unmap_physmem(unlock, MAP_NOCACHE);
	unmap_physmem(mpdg, MAP_NOCACHE);
	unmap_physmem(mpdgs, MAP_NOCACHE);
	unmap_physmem(mpier0, MAP_NOCACHE);
	unmap_physmem(mpimr0, MAP_NOCACHE);
}

static void mp_phy_module_standy_set(u8 clk_reg_no, u8 pos, u8 mode)
{
	void __iomem *unlock = map_physmem(MDLC_PKCPROT1, 4, MAP_NOCACHE);
	void __iomem *msress = map_physmem(MDLC_MSRESS(clk_reg_no), 4, MAP_NOCACHE);
	void __iomem *msres = map_physmem(MDLC_MSRES(clk_reg_no), 4, MAP_NOCACHE);
	u32 val;

	writel(0xA5A5A501, unlock);

	if ((readl(msress) & MPPHY_CLK_MASK(pos)) == (mode << MPPHY_CLK_SHIFT(pos)))
		goto unmap;

	while ((readl(msress) & MPPHY_CLK_MASK(pos)) != (readl(msres) & MPPHY_CLK_MASK(pos)))
		udelay(1000);

	val = readl(msres);
	val &= ~MPPHY_CLK_MASK(pos);
	val |= mode << MPPHY_CLK_SHIFT(pos);
	writel(val, msres);

	while ((readl(msress) & MPPHY_CLK_MASK(pos)) != (readl(msres) & MPPHY_CLK_MASK(pos)))
		udelay(1000);

unmap:
	unmap_physmem(unlock, MAP_NOCACHE);
	unmap_physmem(msress, MAP_NOCACHE);
	unmap_physmem(msres, MAP_NOCACHE);
}

static void mp_phy_module_power_reset(void)
{
	mp_phy_module_power_gating_set(3, 0x03);
	mp_phy_module_power_gating_set(4, 0x03);
	mp_phy_module_power_gating_set(5, 0x03);
	mp_phy_module_power_gating_set(6, 0x03);

	mp_phy_module_standy_set(6, 8, 0x01);
	mp_phy_module_standy_set(6, 10, 0x01);
	mp_phy_module_standy_set(6, 12, 0x01);
	mp_phy_module_standy_set(6, 14, 0x01);
	mp_phy_module_standy_set(6, 16, 0x01);
}

static void mp_phy_module_power_on(void)
{
	mp_phy_module_power_gating_set(3, 0x03);
	mp_phy_module_power_gating_set(4, 0x03);
	mp_phy_module_power_gating_set(5, 0x03);
	mp_phy_module_power_gating_set(6, 0x03);

	mp_phy_module_standy_set(6, 8, 0x03);
	mp_phy_module_standy_set(6, 10, 0x03);
	mp_phy_module_standy_set(6, 12, 0x03);
	mp_phy_module_standy_set(6, 14, 0x03);
	mp_phy_module_standy_set(6, 16, 0x03);
}
#endif
/*-----------------------------------------------------------------------------*/

/* Common registers */
#define MPPHY_CMNCNT1        0x80000
#define MPPHY_CMNCNT2        0x80004
#define MPPHY_PCS0REG1       0x85000
#define MPPHY_PCS0REG5       0x85010

/* Channel register base and offsets */
#define MPPHY_CHAN_BASE(ch)	(0x81000 + (ch) * 0x1000)
#define MPPHY_PXTEST_OFFSET	0x00C
#define MPPHY_RXCNT_OFFSET	0x038
#define MPPHY_SRAMCNT_OFFSET	0x040
#define MPPHY_REFCLK_OFFSET	0x014
#define MPPHY_CNTXT1_OFFSET	0x004
#define MPPHY_CNTXT2_OFFSET	0x008
#define MPPHY_TXREQ_OFFSET	0x044

/* Channel specific registers */
#define MPPHY_PXTEST(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_PXTEST_OFFSET)
#define MPPHY_PXRXCNT(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_RXCNT_OFFSET)
#define MPPHY_PXSRAMCNT(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_SRAMCNT_OFFSET)
#define MPPHY_PXREFCLK(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_REFCLK_OFFSET)
#define MPPHY_PXCNTXT1(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_CNTXT1_OFFSET)
#define MPPHY_PXCNTXT2(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_CNTXT2_OFFSET)
#define MPPHY_PXTXREQ(ch)	(MPPHY_CHAN_BASE(ch) + MPPHY_TXREQ_OFFSET)

/* Channel enable bit masks for MPPHY_CMNCNT1 register */
#define MPPHY_CMNCNT1_CH_MASK(ch)    (0xFF << ((ch) * 8))

/* Channel enable bits for MPPHY_CMNCNT1 register */
#define MPPHY_CMNCNT1_CH_EN(ch)      ((ch) == 0 ? BIT(1) : BIT((ch) * 8))

/* PCS0REG5 register mask and values for each channel */
#define MPPHY_PCS0REG5_CH(ch)        (0x03 << (24 + (ch) * 2))

/* PCS0REG1 register bits */
#define MPPHY_PCS0REG1_VAL         0x00010000

/* PXTEST register bit */
#define MPPHY_PXTEST_BIT            0x1

/* PXRXCNT register reset value */
#define MPPHY_PXRXCNT_RESET_VAL     0x202

/* PXSRAMCNT register bits */
#define MPPHY_PXSRAMCNT_BYPASS      BIT(0)
#define MPPHY_PXSRAMCNT_BIT3        BIT(3)
#define BOOTLOAD_BYPASS_MODE	    0x3
#define SRAM_BYPASS_MODE	    0xC
#define SRAM_EXT_LD_DONE	    0x10
#define SRAM_INIT_DONE		    0x20

#define SRAM_CONTROL_SET_BIT	(BOOTLOAD_BYPASS_MODE | SRAM_BYPASS_MODE | \
				SRAM_EXT_LD_DONE | SRAM_INIT_DONE)

/* CMNCNT1/2 clock settings */
#define MPPHY_CMNCNT2_CLK_CH(ch)     (0x30003 << ((ch) * 4))

/* PXREFCLK register value */
#define MPPHY_PXREFCLK_VAL          0x35

/* PXTXREQ register value */
#define MPPHY_PXTXREQ_VAL           0x8

/* Context settings */
#define MPPHY_CNTXT1_VALUE	0x02010002
#define MPPHY_CNTXT2_VALUE	0x02020202  /* For channels 1-3 */
#define MPPHY_CNTXT2_CH0_VALUE	0x02020201  /* Special for channel 0 */
#define MPPHY_TXREQ_VALUE	0x8

/* struct mpphy_priv - Private data for the MPPHY driver */
struct mp_phy_priv {
	void __iomem *base;
	struct device *dev;
	struct phy *phy;
	struct reset_ctl *reset_ctl;
	struct clk_bulk clks;
	int lane_id;
};

static void mp_phy_write(void __iomem *addr, u32 offs, u32 value)
{
	writel(value, addr + offs);
}

static void mpphy_update_bits(void __iomem *addr, u32 offs, u32 mask, u32 value)
{
	uint32_t tmp, ret;

	ret = readl(addr + offs);
	tmp = ret & ~mask;
	tmp |= value & mask;

	writel(tmp, addr + offs);
}

static int mp_phy_init(struct phy *p)
{
	struct mp_phy_priv *priv = dev_get_priv(p->dev);
	u32 channel_id = p->id;
	u32 cntxt2_val;

	if (channel_id > 3) {
		printf("Invalid channel ID: %d\n", channel_id);
		return -EINVAL;
	}

	cntxt2_val = (channel_id == 0) ? MPPHY_CNTXT2_CH0_VALUE : MPPHY_CNTXT2_VALUE;

	/*
	 * Note: Current source code only supports Ethernet
	 */
	mpphy_update_bits(priv->base, MPPHY_CMNCNT1, MPPHY_CMNCNT1_CH_MASK(channel_id), MPPHY_CMNCNT1_CH_EN(channel_id));
	mpphy_update_bits(priv->base, MPPHY_PCS0REG5, MPPHY_PCS0REG5_CH(channel_id), MPPHY_PCS0REG5_CH(channel_id));
	mpphy_update_bits(priv->base, MPPHY_PCS0REG1, MPPHY_PCS0REG1_VAL, MPPHY_PCS0REG1_VAL);
	mpphy_update_bits(priv->base, MPPHY_PXTEST(channel_id), MPPHY_PXTEST_BIT, MPPHY_PXTEST_BIT);
	mpphy_update_bits(priv->base, MPPHY_PCS0REG5, MPPHY_PCS0REG5_CH(channel_id), 0x0);
	mpphy_update_bits(priv->base, MPPHY_PCS0REG1, MPPHY_PCS0REG1_VAL, 0x0);
	mpphy_update_bits(priv->base, MPPHY_PXTEST(channel_id), MPPHY_PXTEST_BIT, 0x0);

	/* Set PHY rx/tx reset and sram bypass mode */
	mp_phy_write(priv->base, MPPHY_PXRXCNT(channel_id), MPPHY_PXRXCNT_RESET_VAL);
	mp_phy_write(priv->base, MPPHY_PXSRAMCNT(channel_id), MPPHY_PXSRAMCNT_BYPASS);
	mpphy_update_bits(priv->base, MPPHY_PXSRAMCNT(channel_id), MPPHY_PXSRAMCNT_BIT3, MPPHY_PXSRAMCNT_BIT3);

	/* Clock supply settings */
	mpphy_update_bits(priv->base, MPPHY_CMNCNT2, MPPHY_CMNCNT2_CLK_CH(channel_id), MPPHY_CMNCNT2_CLK_CH(channel_id));

	mpphy_update_bits(priv->base, MPPHY_PXREFCLK(channel_id), MPPHY_PXREFCLK_VAL, MPPHY_PXREFCLK_VAL);

	/* Release PHY rx/tx reset */
	mp_phy_write(priv->base, MPPHY_PXRXCNT(channel_id), 0x0);

	/* Setting Context Restore Registers and select PHY2/PHY3 protocol */
	mp_phy_write(priv->base, MPPHY_PXCNTXT1(channel_id), MPPHY_CNTXT1_VALUE);
	mp_phy_write(priv->base, MPPHY_PXCNTXT2(channel_id), cntxt2_val);
	mp_phy_write(priv->base, MPPHY_PXTXREQ(channel_id), MPPHY_PXTXREQ_VAL);

	return 0;
}

static int mp_phy_late_init(struct phy *phy)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);

	u32 channel_id = phy->id;

	/*
	 * The datasheet describes initialization procedure without full
	 * information about the registers. Therefore, the source code is based
	 * on the bare metal code shared by the board team.
	 */

	mp_phy_write(priv->base, MPPHY_PXSRAMCNT(channel_id), SRAM_CONTROL_SET_BIT);

	return 0;
}

static int mp_phy_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	if (mode != PHY_MODE_ETHERNET)
		return -EOPNOTSUPP;

	return 0;
}

static int mp_phy_of_xlate(struct phy *phy,  struct ofnode_phandle_args *args)
{
	struct mp_phy_priv *priv = dev_get_priv(phy->dev);

	if (args->args_count > 2) {
		debug("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	/* Set channel ID from first argument if available */
	if (args->args_count)
		phy->id = args->args[0];
	else
		phy->id = 0;

	/* Set lane ID from second argument if available */
	if (args->args_count > 1)
		priv->lane_id = args->args[1];
	else
		priv->lane_id = 0;  /* Default lane ID if not specified */

	return 0;
}

static const struct phy_ops mp_phy_ops = {
	.init		= mp_phy_init,
	.power_on	= mp_phy_late_init,
	.set_mode	= mp_phy_set_mode,
	.of_xlate	= mp_phy_of_xlate,
};

static int mp_phy_probe(struct udevice *dev)
{
	struct mp_phy_priv *priv = dev_get_priv(dev);
	struct phy phy;
#if !CONFIG_IS_ENABLED(RCAR_SCP_FIXUP)
	int err;
#endif

	/* Get base address from device tree */
	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

#if !CONFIG_IS_ENABLED(RCAR_SCP_FIXUP)
	err = clk_get_bulk(dev, &priv->clks);
	if (err < 0)
		return err;

	err = clk_enable_bulk(&priv->clks);
	if (err)
		goto err_clk_enable;
#endif

#if CONFIG_IS_ENABLED(RCAR_SCP_FIXUP)
	mp_phy_module_power_reset();
	mp_phy_module_power_on();
#endif
	memset(&phy, 0, sizeof(phy));
	phy.dev = dev;

	printf("Multi-Protocol PHY driver probed\n");
	return 0;

#if !CONFIG_IS_ENABLED(RCAR_SCP_FIXUP)
err_clk_enable:
	clk_release_bulk(&priv->clks);
	return err;
#endif
}

static const struct udevice_id mp_phy_ids[] = {
	{ .compatible = "renesas,multi-protocol-phy" },
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

