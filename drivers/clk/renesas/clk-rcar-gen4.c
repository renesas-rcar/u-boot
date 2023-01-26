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
#include <linux/clk-provider.h>

#include <dt-bindings/clock/renesas-cpg-mssr.h>

#include "renesas-cpg-mssr.h"
#include "rcar-gen4-cpg.h"
#include "rcar-cpg-lib.h"

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

static const struct clk_div_table cpg_rpcsrc_div_table[] = {
	{ 0, 4 }, { 1, 6 }, { 2, 5 }, { 3, 6 }, { 0, 0 },
};

static u64 gen4_clk_get_rate64(struct clk *clk);

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

	switch (core->type) {
	case CLK_TYPE_GEN4_SD:
		writel((rate == 400000000) ? 0x4 : 0x1, priv->base + core->offset);
		debug("%s[%i] SDIF offset=%x\n", __func__, __LINE__, core->offset);
		break;
	}

	return 0;
}

static u64 gen4_clk_get_rate64_pll_mul_reg(struct gen4_clk_priv *priv,
					   struct clk *parent,
					   u32 mul_reg, u32 mult, u32 div,
					   char *name)
{
	u32 value;
	u64 rate;

	if (mul_reg) {
		value = readl(priv->base + mul_reg);
		mult = (((value >> 24) & 0x7f) + 1) * 2;
		div = 1;
	}

	rate = gen4_clk_get_rate64(parent) * mult / div;

	debug("%s[%i] %s clk: mult=%u div=%u => rate=%llu\n",
	      __func__, __LINE__, name, mult, div, rate);
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
	u32 div, parent_id;
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
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, 1, pll_config->extal_div,
						"MAIN");

	case CLK_TYPE_GEN4_PLL1:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, pll_config->pll1_mult,
						pll_config->pll1_div, "PLL1");

	case CLK_TYPE_GEN4_PLL2:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, pll_config->pll2_mult,
						pll_config->pll2_div, "PLL2");

	case CLK_TYPE_GEN4_PLL3:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, pll_config->pll3_mult,
						pll_config->pll3_div, "PLL3");

	case CLK_TYPE_GEN4_PLL4:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, pll_config->pll4_mult,
						pll_config->pll4_div, "PLL4");

	case CLK_TYPE_GEN4_PLL5:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, pll_config->pll5_mult,
						pll_config->pll5_div, "PLL5");

	case CLK_TYPE_GEN4_PLL6:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, pll_config->pll6_mult,
						pll_config->pll6_div, "PLL6");

	case CLK_TYPE_GEN4_PLL2X_3X:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						core->offset, 0, 0, "PLL2X_3X");

	case CLK_TYPE_FF:
		return gen4_clk_get_rate64_pll_mul_reg(priv, &parent,
						0, core->mult, core->div,
						"FIXED");

	case CLK_TYPE_GEN4_MDSEL:
		if (priv->cpg_mode & BIT(core->offset)) {
			parent_id = core->parent & 0xffff;
			div = core->div & 0xffff;
		} else {
			parent_id = core->parent >> 16;
			div = core->div >> 16;
		}
		rate = gen4_clk_get_rate64(&parent) / div;
		debug("%s[%i] MDSEL clk: parent=%i div=%u => rate=%llu\n",
		      __func__, __LINE__, parent_id, div, rate);
		return rate;

	case CLK_TYPE_GEN4_SDSRC:
		div = ((readl(priv->base + SD0CKCR1) >> 29) & 0x03) + 4;
		rate = gen4_clk_get_rate64(&parent) / div;
		debug("%s[%i] SDSRC clk: parent=%i div=%u => rate=%llu\n",
		      __func__, __LINE__, core->parent, div, rate);
		return rate;

	case CLK_TYPE_GEN4_SDH:
		return rcar_clk_get_rate64_sdh(core->parent,
					       gen4_clk_get_rate64(&parent),
					       priv->base + core->offset);

	case CLK_TYPE_GEN4_SD:
		return rcar_clk_get_rate64_sd(core->parent,
					      gen4_clk_get_rate64(&parent),
					      priv->base + core->offset);

	case CLK_TYPE_GEN4_RPCSRC:
		return rcar_clk_get_rate64_div_table(core->parent,
						     gen4_clk_get_rate64(&parent),
						     priv->base + CPG_RPCCKCR, 3, 2,
						     cpg_rpcsrc_div_table, "RPCSRC");

	case CLK_TYPE_GEN4_RPC:
		return rcar_clk_get_rate64_rpc(core->parent,
					       gen4_clk_get_rate64(&parent),
					       priv->base + CPG_RPCCKCR);

	case CLK_TYPE_GEN4_RPCD2:
		return rcar_clk_get_rate64_rpcd2(core->parent,
						 gen4_clk_get_rate64(&parent));
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
	u32 cpg_mode;
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

	cpg_mode = readl(rst_base + info->reset_modemr_offset);

	priv->cpg_pll_config =
		(struct rcar_gen4_cpg_pll_config *)info->get_pll_config(cpg_mode);
	if (!priv->cpg_pll_config->extal_div)
		return -EINVAL;

	priv->cpg_mode = cpg_mode;

	if (info->reg_layout == CLK_REG_LAYOUT_RCAR_GEN4) {
		priv->info->status_regs = mstpsr_for_gen4;
		priv->info->control_regs = mstpcr_for_gen4;
		priv->info->reset_regs = srcr_for_gen4;
		priv->info->reset_clear_regs = srstclr_for_gen4;
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
