/* SPDX-License-Identifier: GPL-2.0 */
/*
 * R-Car Gen4 Clock Pulse Generator
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 *
 */

#ifndef __CLK_RENESAS_RCAR_GEN4_CPG_H__
#define __CLK_RENESAS_RCAR_GEN4_CPG_H__

enum rcar_gen4_clk_types {
	CLK_TYPE_GEN4_MAIN = CLK_TYPE_CUSTOM,
	CLK_TYPE_GEN4_MDSEL,		/* Select parent/divider using mode pin */
	CLK_TYPE_GEN4_OSC,		/* OSC EXTAL pre-divider and fixed divider */

	/* SoC specific definitions start here */
	CLK_TYPE_GEN4_SOC_BASE,
};

#define DEF_GEN4_MDSEL(_name, _id, _md, _parent0, _div0, _parent1, _div1) \
	DEF_BASE(_name, _id, CLK_TYPE_GEN4_MDSEL,	\
		 (_parent0) << 16 | (_parent1),		\
		 .div = (_div0) << 16 | (_div1), .offset = _md)

#define DEF_GEN4_OSC(_name, _id, _parent, _div)		\
	DEF_BASE(_name, _id, CLK_TYPE_GEN4_OSC, _parent, .div = _div)

struct rcar_gen4_cpg_pll_config {
	u8 extal_div;
	u8 osc_prediv;
	u8 pll5_mult;
	u8 pll5_div;
};

#define CPG_RST_MODEMR0		0x000
#define CPG_RST_MODEMR1		0x004

#define CPG_PLL1CR0		0x0830
#define CPG_PLL2CR0		0x0834
#define CPG_PLL3CR0		0x083C
#define CPG_PLL4CR0		0x0844
#define CPG_PLL6CR0		0x084C

#define CPG_PLL1CR1		0x08B0
#define CPG_PLL2CR1		0x08B8
#define CPG_PLL3CR1		0x08C0
#define CPG_PLL4CR1		0x08C8
#define CPG_PLL6CR1		0x08D8

#define CPG_SD0CKCR0		0x0870
#define CPG_SD0CKCR1		0x08A4
#define CPG_RPCCKCR		0x0874
#define CPG_MSOCKCR		0x087C

struct gen4_clk_priv {
	void __iomem		*base;
	struct cpg_mssr_info	*info;
	struct clk		clk_extal;
	struct clk		clk_extalr;
	u64			cpg_mode;
	const struct rcar_gen4_cpg_pll_config *cpg_pll_config;
};

int gen4_clk_probe(struct udevice *dev);
int gen4_clk_remove(struct udevice *dev);

extern const struct clk_ops gen4_clk_ops;

#endif
