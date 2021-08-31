// SPDX-License-Identifier: GPL-2.0+
/*
 * Renesas R-Car Gen4 CPG MSSR driver
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 *
 * This file is based on the drivers/clk/renesas/clk-rcar-gen3.c
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <wait_bit.h>
#include <asm/io.h>
#include <linux/bitops.h>

#include <dt-bindings/clock/renesas-cpg-mssr.h>

#include "renesas-cpg-mssr.h"
#include "rcar-gen4-cpg.h"

struct clk_div_table {
	u32 val;
	unsigned int div;
};

/* CPG_RPCFC[4:3] */
static const struct clk_div_table cpg_rpcsrc_div_table[] = {
	{ 2, 5 }, { 3, 6 }, { 0, 0 },
};

/* CPG_RPCFC[2:0] */
static const struct clk_div_table cpg_rpc_div_table[] = {
	{ 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 }, { 0, 0 },
};

/*
 * SDn Clock
 */

/* CPG_SDSRC_SEL[30:29] */
static const struct clk_div_table cpg_sdsrc_div_table[] = {
	{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 0, 0 },
};

#define CPG_SD_STP_HCK		BIT(9)
#define CPG_SD_STP_CK		BIT(8)

#define CPG_SD_STP_MASK		(CPG_SD_STP_HCK | CPG_SD_STP_CK)
#define CPG_SD_FC_MASK		(0x7 << 2 | 0x3 << 0)

#define CPG_SD_DIV_TABLE_DATA(stp_hck, sd_srcfc, sd_fc, sd_div) \
{ \
	.val = ((stp_hck) ? CPG_SD_STP_HCK : 0) | \
	       ((sd_srcfc) << 2) | \
	       ((sd_fc) << 0), \
	.div = (sd_div), \
}

/* SDn divider
 *          sd_srcfc   sd_fc   div
 * stp_hck  (div)      (div)     = sd_srcfc x sd_fc
 *--------------------------------------------------------
 *  0        0 (1)      1 (4)      4
 *  0        1 (2)      1 (4)      8
 *  1        2 (4)      1 (4)     16
 *  1        3 (8)      1 (4)     32
 *  1        4 (16)     1 (4)     64
 *  0        0 (1)      0 (2)      2
 *  0        1 (2)      0 (2)      4
 *  1        2 (4)      0 (2)      8
 *  1        3 (8)      0 (2)     16
 *  1        4 (16)     0 (2)     32
 */
static const struct clk_div_table cpg_sd_div_table[] = {
/*	CPG_SD_DIV_TABLE_DATA(stp_hck,  sd_srcfc,   sd_fc,  sd_div) */
	CPG_SD_DIV_TABLE_DATA(0,        0,          1,        4),
	CPG_SD_DIV_TABLE_DATA(0,        1,          1,        8),
	CPG_SD_DIV_TABLE_DATA(1,        2,          1,       16),
	CPG_SD_DIV_TABLE_DATA(1,        3,          1,       32),
	CPG_SD_DIV_TABLE_DATA(1,        4,          1,       64),
	CPG_SD_DIV_TABLE_DATA(0,        0,          0,        2),
	CPG_SD_DIV_TABLE_DATA(0,        1,          0,        4),
	CPG_SD_DIV_TABLE_DATA(1,        2,          0,        8),
	CPG_SD_DIV_TABLE_DATA(1,        3,          0,       16),
	CPG_SD_DIV_TABLE_DATA(1,        4,          0,       32),
	{ /* Sentinel */ },
};

static int gen4_clk_get_parent(struct gen4_clk_priv *priv, struct clk *clk,
			       struct cpg_mssr_info *info, struct clk *parent)
{
	const struct cpg_core_clk *core;
	int ret;

	if (!renesas_clk_is_mod(clk)) {
		ret = renesas_clk_get_core(clk, info, &core);
		if (ret)
			return ret;

		if (core->type == CLK_TYPE_GEN4_MDSEL) {
			if (priv->cpg_mode & BIT(core->offset))
				parent->id = core->parent & 0xffff;
			else
				parent->id = core->parent >> 16;

			parent->dev = clk->dev;
			return 0;
		}
	}

	return renesas_clk_get_parent(clk, info, parent);
}

static int gen4_clk_setup_sdif_div(struct clk *clk, ulong rate)
{
	struct gen4_clk_priv *priv = dev_get_priv(clk->dev);
	struct cpg_mssr_info *info = priv->info;
	const struct cpg_core_clk *core;
	struct clk parent;
	int ret;

	ret = gen4_clk_get_parent(priv, clk, info, &parent);
	if (ret) {
		printf("%s[%i] parent fail, ret=%i\n", __func__, __LINE__, ret);
		return ret;
	}

	if (renesas_clk_is_mod(&parent))
		return 0;

	ret = renesas_clk_get_core(&parent, info, &core);
	if (ret)
		return ret;

	if (core->type != CLK_TYPE_R8A779F0_SD)
		return 0;

	debug("%s[%i] SDIF offset=%x\n", __func__, __LINE__, core->offset);

	writel((rate == 400000000) ? 0x4 : 0x1, priv->base + core->offset);

	return 0;
}

static int gen4_clk_enable(struct clk *clk)
{
	struct gen4_clk_priv *priv = dev_get_priv(clk->dev);

	return renesas_clk_endisable(clk, priv->base, priv->info, true);
}

static int gen4_clk_disable(struct clk *clk)
{
	struct gen4_clk_priv *priv = dev_get_priv(clk->dev);

	return renesas_clk_endisable(clk, priv->base, priv->info, false);
}

/* PLL mode and output frequency dither mode
 *
 * Value     PLL mode                      Output frequency
 * B'000     Integer multiplication        fixed (0% dither)
 * B'100     Fractional multiplication     fixed (0% dither)
 * B'110     Fractional multiplication     dithered (down spread)
 * B'111     Fractional multiplication     dithered (center spread)
 * Others    Setting prohibited
 */
#define CPG_SSMODE_INT		0x0
#define CPG_SSMODE_FRAC		0x4
#define CPG_SSMODE_FRAC_DOWN	0x6
#define CPG_SSMODE_FRAC_CENTER	0x7

#define is_v4h (rmobile_get_cpu_type() == RMOBILE_CPU_TYPE_R8A779G0)

/* PLLxCR0 */
#define CPG_PLL_KICK		BIT(31)
#define CPG_PLL_NI_S4(val)	((val >> 20) & 0x1ff) /* R-Car S4 */
#define CPG_PLL_NI_V4H(val)	((val >> 20) & 0xff) /* R-Car V4H */
#define CPG_PLL_NI(val)		(is_v4h ? CPG_PLL_NI_V4H(val) : CPG_PLL_NI_S4(val))
#define CPG_PLL_SSMODE(val)	((val >> 16) & 0x7)
#define CPG_PLL_SSFREQ(val)	((val >> 8) & 0x7f)
#define CPG_PLL_SSDEPT(val)	(val & 0x7f)

/* PLLxCR1 */
#define CPG_PLL_NF_S4(val)	(val & 0xffffff) /* R-Car S4 */
#define CPG_PLL_NF_V4H(val)	(val & 0x1ffffff) /* R-Car V4H */
#define CPG_PLL_NF(val)		(is_v4h ? CPG_PLL_NF_V4H(val) : CPG_PLL_NF_S4(val))

static u64 gen4_clk_get_rate64(struct clk *clk);

static u64 gen4_clk_get_rate64_pll_mul_reg(struct gen4_clk_priv *priv,
					   struct clk *parent,
					   const struct cpg_core_clk *core,
					   u32 mul_reg, u32 frac_reg,
					   u32 mult, u32 div, char *name)
{
	u32 value, ssmode = 0, frac = 0;
	u64 rate;

	if (mul_reg) {
		value = readl(priv->base + mul_reg);
		ssmode = CPG_PLL_SSMODE(value);

		frac = (ssmode == CPG_SSMODE_INT || !frac_reg) ? 0 :
			CPG_PLL_NF(readl(priv->base + frac_reg));

		switch (core->type) {
		case CLK_TYPE_R8A779F0_PLL1:
			if (ssmode == CPG_SSMODE_FRAC ||
			    ssmode == CPG_SSMODE_FRAC_CENTER)
				mult = (CPG_PLL_NI(value) + 1);
			else
				return -EINVAL;
			break;

		case CLK_TYPE_R8A779F0_PLL2:
		case CLK_TYPE_R8A779F0_PLL6:
			if (ssmode == CPG_SSMODE_INT)
				mult = (CPG_PLL_NI(value) + 1) * 2;
			else if (ssmode == CPG_SSMODE_FRAC ||
				 ssmode == CPG_SSMODE_FRAC_DOWN)
				mult = (CPG_PLL_NI(value) + 1);
			else
				return -EINVAL;
			break;

		case CLK_TYPE_R8A779F0_PLL3:
			if (ssmode == CPG_SSMODE_INT)
				mult = (CPG_PLL_NI(value) + 1) * 2;
			else
				return -EINVAL;
			break;

		case CLK_TYPE_R8A779G0_PLL1:
		case CLK_TYPE_R8A779G0_PLL2:
		case CLK_TYPE_R8A779G0_PLL4:
		case CLK_TYPE_R8A779G0_PLL6:
			if (ssmode == CPG_SSMODE_INT ||
			    ssmode == CPG_SSMODE_FRAC ||
			    ssmode == CPG_SSMODE_FRAC_DOWN)
				mult = (CPG_PLL_NI(value) + 1) * 2;
			else
				return -EINVAL;
			break;

		case CLK_TYPE_R8A779G0_PLL3:
			if (ssmode == CPG_SSMODE_INT)
				mult = (CPG_PLL_NI(value) + 1) * 2;
			else
				return -EINVAL;
		}
	}

	rate = gen4_clk_get_rate64(parent) * (mult + frac/0xffffff) / div;

	debug("%s[%i] %s clk: parent=%i mult=%u div=%u mode=%u => rate=%llu\n",
	      __func__, __LINE__, name, core->parent, mult, div, ssmode, rate);
	return rate;
}

static unsigned int _get_table_div(const struct clk_div_table *table, u32 value)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == value)
			return clkt->div;
	return 0;
}

/*
 * @reg: register address to adjust divider
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @table: array of divider/value pairs ending with a div set to 0
 */
#define div_mask(width)	((1 << (width)) - 1)
#define RPC_CMNCR		0xEE200000
#define RPC_CMNCR_MOIIO3(val)	(((val) & 0x3) << 22)
#define RPC_CMNCR_MOIIO2(val)	(((val) & 0x3) << 20)
#define RPC_CMNCR_MOIIO1(val)	(((val) & 0x3) << 18)
#define RPC_CMNCR_MOIIO0(val)	(((val) & 0x3) << 16)
#define RPC_CMNCR_MOIIO_HIZ	(RPC_CMNCR_MOIIO0(3) | RPC_CMNCR_MOIIO1(3) | \
				 RPC_CMNCR_MOIIO2(3) | RPC_CMNCR_MOIIO3(3))

static u64 gen4_clk_get_rate64_div_table(struct gen4_clk_priv *priv,
					 struct clk *parent,
					 const struct cpg_core_clk *core,
					 u32 reg, u32 shift, u32 width,
					 const struct clk_div_table *table,
					 char *name)
{
	u32 value, div;
	u64 rate;

retry:
	value = readl(priv->base + reg) >> shift;
	value &= div_mask(width);

	if ((value == 0x3) && (core->type == CLK_TYPE_R8A779F0_RPCSRC ||
	    core->type == CLK_TYPE_R8A779G0_RPCSRC)) {
		debug("%s: %s: force to correct RPCFC[4:3] to 0x2\n",
		      __func__, name);
		/*
		 * When the operation frequency changes by setting RPCCKCR in
		 * CPG, read after write operation on this register is
		 * necessary to guarantee the setting to be applied.
		 * Before changing RPCCKCR setting, set CMNCR.MOIIOx(x=0,1,2,3)
		 * register to B’11 (Hi-Z).
		 */
		writel(readl(RPC_CMNCR) | RPC_CMNCR_MOIIO_HIZ, RPC_CMNCR);
		value = readl(priv->base + reg) & 0xFFFFFFF7;
		writel(value, priv->base + reg);
		readl(priv->base + reg);
		goto retry;
	}

	div = _get_table_div(table, value);
	if (!div)
		return -EINVAL;

	rate = gen4_clk_get_rate64(parent) / div;
	debug("%s[%i] %s clk: parent=%i div=%u => rate=%llu\n",
	      __func__, __LINE__, name, core->parent, div, rate);

	return rate;
}

static u64 gen4_clk_get_rate64(struct clk *clk)
{
	struct gen4_clk_priv *priv = dev_get_priv(clk->dev);
	struct cpg_mssr_info *info = priv->info;
	struct clk parent;
	const struct cpg_core_clk *core;
	const struct rcar_gen4_cpg_pll_config *pll_config =
					priv->cpg_pll_config;
	u32 value, div, parent_id;
	u64 rate = 0;
	int ret;

	debug("%s[%i] Clock: id=%lu\n", __func__, __LINE__, clk->id);

	ret = gen4_clk_get_parent(priv, clk, info, &parent);
	if (ret) {
		printf("%s[%i] parent fail, ret=%i\n", __func__, __LINE__, ret);
		return ret;
	}

	if (renesas_clk_is_mod(clk)) {
		rate = gen4_clk_get_rate64(&parent);
		debug("%s[%i] MOD clk: parent=%lu => rate=%llu\n",
		      __func__, __LINE__, parent.id, rate);
		return rate;
	}

	ret = renesas_clk_get_core(clk, info, &core);
	if (ret)
		return ret;

	switch (core->type) {
	case CLK_TYPE_IN:
		if (core->id == info->clk_extal_id) {
			rate = clk_get_rate(&priv->clk_extal);
			debug("%s[%i] EXTAL clk: rate=%llu\n",
			      __func__, __LINE__, rate);
			return rate;
		}

		if (core->id == info->clk_extalr_id) {
			rate = clk_get_rate(&priv->clk_extalr);
			debug("%s[%i] EXTALR clk: rate=%llu\n",
			      __func__, __LINE__, rate);
			return rate;
		}

		return -EINVAL;

	case CLK_TYPE_GEN4_MAIN:
	case CLK_TYPE_R8A779F0_MAIN:
	case CLK_TYPE_R8A779G0_MAIN:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					0, 0, 1, pll_config->extal_div,
					"MAIN");

	case CLK_TYPE_R8A779F0_PLL1:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL1CR0, CPG_PLL1CR1, 0,
					pll_config->extal_div, "S4_PLL1");

	case CLK_TYPE_R8A779F0_PLL2:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL2CR0, CPG_PLL2CR1, 0,
					pll_config->extal_div, "S4_PLL2");

	case CLK_TYPE_R8A779F0_PLL3:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL3CR0, CPG_PLL3CR1, 0,
					pll_config->extal_div, "S4_PLL3");

	case CLK_TYPE_R8A779F0_PLL5:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					0, 0, pll_config->pll5_mult,
					pll_config->pll5_div, "S4_PLL5");

	case CLK_TYPE_R8A779F0_PLL6:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL6CR0, CPG_PLL6CR1, 0,
					pll_config->extal_div, "S4_PLL6");

	case CLK_TYPE_R8A779G0_PLL1:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL1CR0, CPG_PLL1CR1, 0,
					pll_config->extal_div, "V4H_PLL1");

	case CLK_TYPE_R8A779G0_PLL2:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL2CR0, CPG_PLL2CR1, 0,
					pll_config->extal_div, "V4H_PLL2");

	case CLK_TYPE_R8A779G0_PLL3:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL3CR0, CPG_PLL3CR1, 0,
					pll_config->extal_div, "V4H_PLL3");

	case CLK_TYPE_R8A779G0_PLL4:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL4CR0, CPG_PLL4CR1, 0,
					pll_config->extal_div, "V4H_PLL4");

	case CLK_TYPE_R8A779G0_PLL5:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					0, 0, pll_config->pll5_mult,
					pll_config->pll5_div, "V4H_PLL5");

	case CLK_TYPE_R8A779G0_PLL6:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					CPG_PLL6CR0, CPG_PLL6CR1, 0,
					pll_config->extal_div, "V4H_PLL6");

	case CLK_TYPE_FF:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					0, 0, core->mult, core->div,
					"FIXED");

	case CLK_TYPE_GEN4_MDSEL:
	case CLK_TYPE_R8A779F0_MDSEL:
	case CLK_TYPE_R8A779G0_MDSEL:
		if (priv->cpg_mode & BIT(core->offset)) {
			parent_id = core->parent & 0xffff;
			div = core->div & 0xffff;
		} else {
			parent_id = core->parent >> 16;
			div = core->div >> 16;
		}
		rate = gen4_clk_get_rate64(&parent) / div;
		debug("%s[%i] MDSEL clk: parent=%i div=%u => rate=%llu\n",
		      __func__, __LINE__,
		      parent_id, div, rate);
		return rate;

	case CLK_TYPE_R8A779F0_SDSCR:
		/* CPG_SDSRC_SEL[30:29] */
		return gen4_clk_get_rate64_div_table(priv, &parent, core,
					core->offset, 29, 2, cpg_sdsrc_div_table,
					"S4_SDSCR");

	case CLK_TYPE_R8A779F0_SD:
		value = readl(priv->base + core->offset);
		value &= CPG_SD_STP_MASK | CPG_SD_FC_MASK;

		div = _get_table_div(cpg_sd_div_table, value);
		if (!div)
			return -EINVAL;

		rate = gen4_clk_get_rate64(&parent) / div;
		debug("%s[%i] SD clk: parent=%i div=%u => rate=%llu\n",
		      __func__, __LINE__, core->parent, div, rate);
		return rate;

	case CLK_TYPE_R8A779F0_RPCSRC:
		/* CPG_RPCFC[4:3] */
		return gen4_clk_get_rate64_div_table(priv, &parent, core,
					core->offset, 3, 2, cpg_rpcsrc_div_table,
					"S4_RPCSRC");

	case CLK_TYPE_R8A779F0_RPC:
		/* CPG_RPCFC[2:0] */
		return gen4_clk_get_rate64_div_table(priv, &parent, core,
					core->offset, 0, 3, cpg_rpc_div_table,
					"S4_RPC");

	case CLK_TYPE_R8A779G0_RPCSRC:
		/* CPG_RPCFC[4:3] */
		return gen4_clk_get_rate64_div_table(priv, &parent, core,
					core->offset, 3, 2, cpg_rpcsrc_div_table,
					"V4H_RPCSRC");

	case CLK_TYPE_R8A779G0_RPC:
		/* CPG_RPCFC[2:0] */
		return gen4_clk_get_rate64_div_table(priv, &parent, core,
					core->offset, 0, 3, cpg_rpc_div_table,
					"V4H_RPC");

	case CLK_TYPE_R8A779F0_RPCD2:
	case CLK_TYPE_R8A779G0_RPCD2:
		rate = gen4_clk_get_rate64(&parent) / core->div;
		debug("%s[%i] RPCD2 clk: parent=%i div=%u => rate=%llu\n",
		      __func__, __LINE__,
		      parent_id, core->div, rate);
		return rate;
	}

	printf("%s[%i] unknown fail\n", __func__, __LINE__);

	return -ENOENT;
}

static ulong gen4_clk_get_rate(struct clk *clk)
{
	return gen4_clk_get_rate64(clk);
}

static ulong gen4_clk_set_rate(struct clk *clk, ulong rate)
{
	/* Force correct SD-IF divider configuration if applicable */
	gen4_clk_setup_sdif_div(clk, rate);
	return gen4_clk_get_rate64(clk);
}

static int gen4_clk_of_xlate(struct clk *clk, struct ofnode_phandle_args *args)
{
	if (args->args_count != 2) {
		debug("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	clk->id = (args->args[0] << 16) | args->args[1];

	return 0;
}

const struct clk_ops gen4_clk_ops = {
	.enable		= gen4_clk_enable,
	.disable	= gen4_clk_disable,
	.get_rate	= gen4_clk_get_rate,
	.set_rate	= gen4_clk_set_rate,
	.of_xlate	= gen4_clk_of_xlate,
};

int gen4_clk_probe(struct udevice *dev)
{
	struct gen4_clk_priv *priv = dev_get_priv(dev);
	struct cpg_mssr_info *info =
		(struct cpg_mssr_info *)dev_get_driver_data(dev);
	fdt_addr_t rst_base;
	u32 cpg_mode0, cpg_mode1;
	u64 cpg_mode;
	int ret;

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	priv->info = info;
	ret = fdt_node_offset_by_compatible(gd->fdt_blob, -1, info->reset_node);
	if (ret < 0)
		return ret;

	rst_base = fdtdec_get_addr(gd->fdt_blob, ret, "reg");
	if (rst_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	cpg_mode0 = readl(rst_base + info->reset_modemr_offset);
	cpg_mode1 = readl(rst_base + info->reset_modemr1_offset);
	cpg_mode = ((u64)cpg_mode1 << 32) | cpg_mode0;

	priv->cpg_pll_config =
		(struct rcar_gen4_cpg_pll_config *)info->get_pll_config(cpg_mode0);
	if (!priv->cpg_pll_config->extal_div)
		return -EINVAL;

	priv->cpg_mode = cpg_mode;

	if (info->reg_layout == CLK_REG_LAYOUT_RCAR_S4) {
		priv->info->status_regs = mstpsr_for_s4;
		priv->info->control_regs = mstpcr_for_s4;
		priv->info->reset_regs = srcr_for_s4;
		priv->info->reset_clear_regs = srstclr_for_s4;
	} else {
		return -EINVAL;
	}

	ret = clk_get_by_name(dev, "extal", &priv->clk_extal);
	if (ret < 0)
		return ret;

	if (info->extalr_node) {
		ret = clk_get_by_name(dev, info->extalr_node, &priv->clk_extalr);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int gen4_clk_remove(struct udevice *dev)
{
	struct gen4_clk_priv *priv = dev_get_priv(dev);

	return renesas_clk_remove(priv->base, priv->info);
}
