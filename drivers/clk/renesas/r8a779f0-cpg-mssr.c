// SPDX-License-Identifier: GPL-2.0
/*
 * r8a779f0 Clock Pulse Generator / Module Standby and Software Reset
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 *
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>

#include <dt-bindings/clock/r8a779f0-cpg-mssr.h>

#include "renesas-cpg-mssr.h"
#include "rcar-gen4-cpg.h"

enum clk_ids {
	/* Core Clock Outputs exported to DT */
	LAST_DT_CORE_CLK = R8A779F0_CLK_OSC,

	/* External Input Clocks */
	CLK_EXTAL,
	CLK_EXTALR,

	/* Internal Core Clocks */
	CLK_MAIN,
	CLK_PLL1,
	CLK_PLL2,
	CLK_PLL3,
	CLK_PLL5,
	CLK_PLL6,
	CLK_PLL1_DIV2,
	CLK_PLL2_DIV2,
	CLK_PLL3_DIV2,
	CLK_PLL5_DIV2,
	CLK_PLL5_DIV4,
	CLK_PLL6_DIV2,
	CLK_S0,
	CLK_SDSRC,
	CLK_RPCSRC,
	CLK_OCO,

	/* Module Clocks */
	MOD_CLK_BASE
};

#define DEF_SDSCR(_name, _id, _parent, _offset)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779F0_SDSCR, _parent, .offset = _offset)

#define DEF_SD(_name, _id, _parent, _offset)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779F0_SD, _parent, .offset = _offset)

#define DEF_RPC(_name, _id, _parent, _offset)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779F0_RPC, _parent, .offset = _offset)

#define DEF_RPCD2(_name, _id, _parent)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779F0_RPCD2, _parent, .div = 2)

#define DEF_MDSEL(_name, _id, _md, _parent0, _div0, _parent1, _div1) \
	DEF_BASE(_name, _id, CLK_TYPE_R8A779F0_MDSEL,	\
		 (_parent0) << 16 | (_parent1),		\
		 .div = (_div0) << 16 | (_div1), .offset = _md)

#define DEF_OSC(_name, _id, _parent, _div)		\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779F0_OSC, _parent, .div = _div)

static const struct cpg_core_clk r8a779f0_core_clks[] = {
	/* External Clock Inputs */
	DEF_INPUT("extal",  CLK_EXTAL),
	DEF_INPUT("extalr", CLK_EXTALR),

	/* Internal Core Clocks */
	DEF_BASE(".main", CLK_MAIN,	CLK_TYPE_R8A779F0_MAIN, CLK_EXTAL),
	DEF_BASE(".pll1", CLK_PLL1,	CLK_TYPE_R8A779F0_PLL1, CLK_MAIN),
	DEF_BASE(".pll3", CLK_PLL3,	CLK_TYPE_R8A779F0_PLL3, CLK_MAIN),
	DEF_BASE(".pll5", CLK_PLL5,	CLK_TYPE_R8A779F0_PLL5, CLK_MAIN),
	DEF_BASE(".pll2", CLK_PLL2,	CLK_TYPE_R8A779F0_PLL2, CLK_MAIN),
	DEF_BASE(".pll6", CLK_PLL6,	CLK_TYPE_R8A779F0_PLL6, CLK_MAIN),

	DEF_FIXED(".pll1_div2",		CLK_PLL1_DIV2,	CLK_PLL1,	2, 1),
	DEF_FIXED(".pll2_div2",		CLK_PLL2_DIV2,	CLK_PLL2,	2, 1),
	DEF_FIXED(".pll3_div2",		CLK_PLL3_DIV2,	CLK_PLL3,	2, 1),
	DEF_FIXED(".pll5_div2",		CLK_PLL5_DIV2,	CLK_PLL5,	2, 1),
	DEF_FIXED(".pll5_div4",		CLK_PLL5_DIV4,	CLK_PLL5_DIV2,	2, 1),
	DEF_FIXED(".pll6_div2",		CLK_PLL6_DIV2,	CLK_PLL6,	2, 1),
	DEF_FIXED(".s0",		CLK_S0,		CLK_PLL1_DIV2,	2, 1),
	DEF_SDSCR(".sdsrc",		CLK_SDSRC,	CLK_PLL5, CPG_SD0CKCR1),
	DEF_RATE(".oco",		CLK_OCO,	32768),

	DEF_BASE(".rpcsrc",	CLK_RPCSRC, CLK_TYPE_R8A779F0_RPCSRC, CLK_PLL5),
	DEF_RPC("rpc",		R8A779F0_CLK_RPC,	CLK_RPCSRC, CPG_RPCCKCR),
	DEF_RPCD2("rpcd2",	R8A779F0_CLK_RPCD2,	R8A779F0_CLK_RPC),

	/* Core Clock Outputs */
	DEF_FIXED("s0d2",	R8A779F0_CLK_S0D2,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3",	R8A779F0_CLK_S0D3,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4",	R8A779F0_CLK_S0D4,	CLK_S0,		4, 1),
	DEF_FIXED("cl16m",	R8A779F0_CLK_CL16M,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_mm",	R8A779F0_CLK_S0D2_MM,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3_mm",	R8A779F0_CLK_S0D3_MM,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4_mm",	R8A779F0_CLK_S0D4_MM,	CLK_S0,		4, 1),
	DEF_FIXED("cl16m_mm",	R8A779F0_CLK_CL16M_MM,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_rt",	R8A779F0_CLK_S0D2_RT,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3_rt",	R8A779F0_CLK_S0D3_RT,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4_rt",	R8A779F0_CLK_S0D4_RT,	CLK_S0,		4, 1),
	DEF_FIXED("s0d6_rt",	R8A779F0_CLK_S0D6_RT,	CLK_S0,		6, 1),
	DEF_FIXED("cl16m_rt",	R8A779F0_CLK_CL16M_RT,	CLK_S0,		48, 1),
	DEF_FIXED("s0d3_per",	R8A779F0_CLK_S0D3_PER,	CLK_S0,		3, 1),
	DEF_FIXED("s0d6_per",	R8A779F0_CLK_S0D6_PER,	CLK_S0,		6, 1),
	DEF_FIXED("s0d12_per",	R8A779F0_CLK_S0D12_PER,	CLK_S0,		12, 1),
	DEF_FIXED("s0d24_per",	R8A779F0_CLK_S0D24_PER,	CLK_S0,		24, 1),
	DEF_FIXED("cl16m_per",	R8A779F0_CLK_CL16M_PER,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_hsc",	R8A779F0_CLK_S0D2_HSC,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3_hsc",	R8A779F0_CLK_S0D3_HSC,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4_hsc",	R8A779F0_CLK_S0D4_HSC,	CLK_S0,		4, 1),
	DEF_FIXED("s0d6_hsc",	R8A779F0_CLK_S0D6_HSC,	CLK_S0,		6, 1),
	DEF_FIXED("s0d12_hsc",	R8A779F0_CLK_S0D12_HSC,	CLK_S0,		12, 1),
	DEF_FIXED("cl16m_hsc",	R8A779F0_CLK_CL16M_HSC,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_cc",	R8A779F0_CLK_S0D2_CC,	CLK_S0,		2, 1),
	DEF_FIXED("rsw2",	R8A779F0_CLK_RSW2,	CLK_PLL5_DIV2,	5, 1),
	DEF_FIXED("cbfusa",	R8A779F0_CLK_CBFUSA,	CLK_EXTAL,	2, 1),
	DEF_FIXED("cpex",	R8A779F0_CLK_CPEX,	CLK_EXTAL,	2, 1),

	DEF_SD("sd0",		R8A779F0_CLK_SD0,	CLK_SDSRC, CPG_SD0CKCR0),

	DEF_DIV6P1("mso",	R8A779F0_CLK_MSO,	CLK_PLL5_DIV4, CPG_MSOCKCR),

	DEF_OSC("osc",		R8A779F0_CLK_OSC,	CLK_EXTAL,	8),
	DEF_MDSEL("r",		R8A779F0_CLK_R, 29, CLK_EXTALR, 1, CLK_OCO, 1),
};

static const struct mssr_mod_clk r8a779f0_mod_clks[] = {
	DEF_MOD("hscif0",	514,	R8A779F0_CLK_S0D3),
	DEF_MOD("hscif1",	515,	R8A779F0_CLK_S0D3),
	DEF_MOD("hscif2",	516,	R8A779F0_CLK_S0D3),
	DEF_MOD("hscif3",	517,	R8A779F0_CLK_S0D3),
	DEF_MOD("i2c0",		518,	R8A779F0_CLK_S0D6_PER),
	DEF_MOD("i2c1",		519,	R8A779F0_CLK_S0D6_PER),
	DEF_MOD("i2c2",		520,	R8A779F0_CLK_S0D6_PER),
	DEF_MOD("i2c3",		521,	R8A779F0_CLK_S0D6_PER),
	DEF_MOD("i2c4",		522,	R8A779F0_CLK_S0D6_PER),
	DEF_MOD("i2c5",		523,	R8A779F0_CLK_S0D6_PER),
	DEF_MOD("msi0",		618,	R8A779F0_CLK_MSO),
	DEF_MOD("msi1",		619,	R8A779F0_CLK_MSO),
	DEF_MOD("msi2",		620,	R8A779F0_CLK_MSO),
	DEF_MOD("msi3",		621,	R8A779F0_CLK_MSO),
	DEF_MOD("pcie0",	624,	R8A779F0_CLK_S0D2),
	DEF_MOD("pcie1",	625,	R8A779F0_CLK_S0D2),
	DEF_MOD("rpc",		629,	R8A779F0_CLK_RPCD2),
	DEF_MOD("rtdm0",	630,	R8A779F0_CLK_S0D2),
	DEF_MOD("rtdm1",	631,	R8A779F0_CLK_S0D2),
	DEF_MOD("rtdm2",	700,	R8A779F0_CLK_S0D2),
	DEF_MOD("rtdm3",	701,	R8A779F0_CLK_S0D2),
	DEF_MOD("scif0",	702,	R8A779F0_CLK_S0D12_PER),
	DEF_MOD("scif1",	703,	R8A779F0_CLK_S0D12_PER),
	DEF_MOD("scif3",	704,	R8A779F0_CLK_S0D12_PER),
	DEF_MOD("scif4",	705,	R8A779F0_CLK_S0D12_PER),
	DEF_MOD("sdhi0",	706,	R8A779F0_CLK_SD0),
	DEF_MOD("sydm1",	709,	R8A779F0_CLK_S0D3),
	DEF_MOD("sydm2",	710,	R8A779F0_CLK_S0D3),
	DEF_MOD("tmu0",		713,	R8A779F0_CLK_S0D6_RT),
	DEF_MOD("tmu1",		714,	R8A779F0_CLK_S0D6_RT),
	DEF_MOD("tmu2",		715,	R8A779F0_CLK_S0D6_RT),
	DEF_MOD("tmu3",		716,	R8A779F0_CLK_S0D6_RT),
	DEF_MOD("tmu4",		717,	R8A779F0_CLK_S0D6_RT),
	DEF_MOD("wdt",		907,	R8A779F0_CLK_CL16M_RT),
	DEF_MOD("cmt0",		910,	R8A779F0_CLK_R),
	DEF_MOD("cmt1",		911,	R8A779F0_CLK_R),
	DEF_MOD("cmt2",		912,	R8A779F0_CLK_R),
	DEF_MOD("cmt3",		913,	R8A779F0_CLK_R),
	DEF_MOD("pfc0",		915,	R8A779F0_CLK_CL16M),
	DEF_MOD("rsw2",		1505,	R8A779F0_CLK_RSW2),
	DEF_MOD("ethphy",	1506,	R8A779F0_CLK_S0D2),
	DEF_MOD("ufs0",		1514,	R8A779F0_CLK_S0D4),
};

/*
 * CPG Clock Data
 */

/*
 * MD		EXTAL		OSC	PLL5
 * 14 13	(MHz)
 *-----------------------------------------
 * 0  0		16.00 x 1	/15	x200
 * 0  1		20.00 x 1	/19	x160
 * 1  0		Prohibited setting
 * 1  1		40.00 / 2	/38	x160
 */

#define CPG_PLL_CONFIG_INDEX(md)	((((md) & BIT(14)) >> 13) | \
					 (((md) & BIT(13)) >> 13))

static const struct rcar_gen4_cpg_pll_config cpg_pll_configs[4] = {
	/* EXTAL div	OSC prediv	PLL5 mult/div */
	{ 1,		15,		200,	1, },
	{ 1,		19,		160,	1, },
	{ 0,		0,		0,	0, },
	{ 2,		38,		160,	1, },
};

/*
 * Note that the only clock left running before booting Linux are now
 * MFIS, INTC-AP, INTC-EX and SCIF3 on S4
 */
#define MSTPCR7_SCIF3	BIT(4) /* No information: MFIS, INTC-AP, INTC-EX */
static const struct mstp_stop_table r8a779f0_mstp_table[] = {
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00800000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x0003c000, 0x0, 0x0, 0x0 },
	{ 0x03000000, 0x0, 0x0, 0x0 },
	{ 0x1ffbe040, MSTPCR7_SCIF3, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00003c78, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x9e800000, 0x0, 0x0, 0x0 },
	{ 0x00000027, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00005800, 0x0, 0x0, 0x0 },
};

static const void *r8a779f0_get_pll_config(const u32 cpg_mode)
{
	return &cpg_pll_configs[CPG_PLL_CONFIG_INDEX(cpg_mode)];
}

static const struct cpg_mssr_info r8a779f0_cpg_mssr_info = {
	.core_clk		= r8a779f0_core_clks,
	.core_clk_size		= ARRAY_SIZE(r8a779f0_core_clks),
	.mod_clk		= r8a779f0_mod_clks,
	.mod_clk_size		= ARRAY_SIZE(r8a779f0_mod_clks),
	.mstp_table		= r8a779f0_mstp_table,
	.mstp_table_size	= ARRAY_SIZE(r8a779f0_mstp_table),
	.reset_node		= "renesas,r8a779f0-rst",
	.reset_modemr_offset	= CPG_RST_MODEMR0,
	.reset_modemr1_offset	= CPG_RST_MODEMR1,
	.extalr_node		= "extalr",
	.mod_clk_base		= MOD_CLK_BASE,
	.clk_extal_id		= CLK_EXTAL,
	.clk_extalr_id		= CLK_EXTALR,
	.get_pll_config		= r8a779f0_get_pll_config,
	.reg_layout		= CLK_REG_LAYOUT_RCAR_S4,
};

static const struct udevice_id r8a779f0_clk_ids[] = {
	{
		.compatible	= "renesas,r8a779f0-cpg-mssr",
		.data		= (ulong)&r8a779f0_cpg_mssr_info
	},
	{ }
};

U_BOOT_DRIVER(clk_r8a779f0) = {
	.name		= "clk_r8a779f0",
	.id		= UCLASS_CLK,
	.of_match	= r8a779f0_clk_ids,
	.priv_auto_alloc_size = sizeof(struct gen4_clk_priv),
	.ops		= &gen4_clk_ops,
	.probe		= gen4_clk_probe,
	.remove		= gen4_clk_remove,
};
