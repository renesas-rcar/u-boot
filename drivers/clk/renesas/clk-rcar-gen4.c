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

/* PLLxCR0 */
#define CPG_PLL_KICK		BIT(31)
#define CPG_PLL_NI_T1(val)	((val >> 20) & 0x1ff) /* R-Car S4 */
#define CPG_PLL_NI_T2(val)	((val >> 20) & 0xff) /* R-Car V4H */
#define CPG_PLL_SSMODE(val)	((val >> 16) & 0x7)
#define CPG_PLL_SSFREQ(val)	((val >> 8) & 0x7f)
#define CPG_PLL_SSDEPT(val)	(val & 0x7f)

/* PLLxCR1 */
#define CPG_PLL_NF_T1(val)	(val & 0xffffff) /* R-Car S4 */
#define CPG_PLL_NF_T2(val)	(val & 0x1ffffff) /* R-Car V4H */

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
			CPG_PLL_NF_T1(readl(priv->base + frac_reg));

		switch (core->type) {
		case CLK_TYPE_R8A779F0_PLL1:
			if (ssmode == CPG_SSMODE_FRAC ||
			    ssmode == CPG_SSMODE_FRAC_CENTER)
				mult = (CPG_PLL_NI_T1(value) + 1);
			else
				return -EINVAL;
			break;

		case CLK_TYPE_R8A779F0_PLL2:
		case CLK_TYPE_R8A779F0_PLL6:
			if (ssmode == CPG_SSMODE_INT)
				mult = (CPG_PLL_NI_T1(value) + 1) * 2;
			else if (ssmode == CPG_SSMODE_FRAC ||
				 ssmode == CPG_SSMODE_FRAC_DOWN)
				mult = (CPG_PLL_NI_T1(value) + 1);
			else
				return -EINVAL;
			break;

		case CLK_TYPE_R8A779F0_PLL3:
			if (ssmode == CPG_SSMODE_INT)
				mult = (CPG_PLL_NI_T1(value) + 1) * 2;
			else
				return -EINVAL;
		}
	}

	rate = gen4_clk_get_rate64(parent) * (mult + frac/0xffffff) / div;

	debug("%s[%i] %s clk: parent=%i mult=%u div=%u mode=%u => rate=%llu\n",
	      __func__, __LINE__, name, core->parent, mult, div, ssmode, rate);
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

	case CLK_TYPE_FF:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent, core,
					0, 0, core->mult, core->div,
					"FIXED");

	case CLK_TYPE_GEN4_MDSEL:
	case CLK_TYPE_R8A779F0_MDSEL:
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
