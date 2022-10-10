// SPDX-License-Identifier: GPL-2.0
/*
 * r8a779g0 Clock Pulse Generator / Module Standby and Software Reset
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 *
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>

#include <dt-bindings/clock/r8a779g0-cpg-mssr.h>

#include "renesas-cpg-mssr.h"
#include "rcar-gen4-cpg.h"

enum clk_ids {
	/* Core Clock Outputs exported to DT */
	LAST_DT_CORE_CLK = R8A779G0_CLK_OSC,

	/* External Input Clocks */
	CLK_EXTAL,
	CLK_EXTALR,

	/* Internal Core Clocks */
	CLK_MAIN,
	CLK_PLL1,
	CLK_PLL2,
	CLK_PLL3,
	CLK_PLL4,
	CLK_PLL5,
	CLK_PLL6,
	CLK_PLL1_DIV2,
	CLK_PLL2_DIV2,
	CLK_PLL3_DIV2,
	CLK_PLL4_DIV2,
	CLK_PLL5_DIV2,
	CLK_PLL5_DIV4,
	CLK_PLL6_DIV2,
	CLK_S0,
	CLK_SASYNCPER,
	CLK_SV_IR,
	CLK_SV_VIP,
	CLK_SDSRC,
	CLK_RPCSRC,
	CLK_OCO,

	/* Module Clocks */
	MOD_CLK_BASE
};

#define DEF_SDSCR(_name, _id, _parent, _offset)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779G0_SDSCR, _parent, .offset = _offset)

#define DEF_SD(_name, _id, _parent, _offset)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779G0_SD, _parent, .offset = _offset)

#define DEF_RPC(_name, _id, _parent, _offset)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779G0_RPC, _parent, .offset = _offset)

#define DEF_RPCD2(_name, _id, _parent)	\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779G0_RPCD2, _parent, .div = 2)

#define DEF_MDSEL(_name, _id, _md, _parent0, _div0, _parent1, _div1) \
	DEF_BASE(_name, _id, CLK_TYPE_R8A779G0_MDSEL,	\
		 (_parent0) << 16 | (_parent1),		\
		 .div = (_div0) << 16 | (_div1), .offset = _md)

#define DEF_OSC(_name, _id, _parent, _div)		\
	DEF_BASE(_name, _id, CLK_TYPE_R8A779G0_OSC, _parent, .div = _div)

static const struct cpg_core_clk r8a779g0_core_clks[] = {
	/* External Clock Inputs */
	DEF_INPUT("extal",  CLK_EXTAL),
	DEF_INPUT("extalr", CLK_EXTALR),

	/* Internal Core Clocks */
	DEF_BASE(".main", CLK_MAIN,	CLK_TYPE_R8A779G0_MAIN, CLK_EXTAL),
	DEF_BASE(".pll1", CLK_PLL1,	CLK_TYPE_R8A779G0_PLL1, CLK_MAIN),
	DEF_BASE(".pll2", CLK_PLL2,	CLK_TYPE_R8A779G0_PLL2, CLK_MAIN),
	DEF_BASE(".pll3", CLK_PLL3,	CLK_TYPE_R8A779G0_PLL3, CLK_MAIN),
	DEF_BASE(".pll4", CLK_PLL4,	CLK_TYPE_R8A779G0_PLL4, CLK_MAIN),
	DEF_BASE(".pll5", CLK_PLL5,	CLK_TYPE_R8A779G0_PLL5, CLK_MAIN),
	DEF_BASE(".pll6", CLK_PLL6,	CLK_TYPE_R8A779G0_PLL6, CLK_MAIN),

	DEF_FIXED(".pll1_div2",		CLK_PLL1_DIV2,	CLK_PLL1,	2, 1),
	DEF_FIXED(".pll2_div2",		CLK_PLL2_DIV2,	CLK_PLL2,	2, 1),
	DEF_FIXED(".pll3_div2",		CLK_PLL3_DIV2,	CLK_PLL3,	2, 1),
	DEF_FIXED(".pll4_div2",		CLK_PLL4_DIV2,	CLK_PLL4,	2, 1),
	DEF_FIXED(".pll5_div2",		CLK_PLL5_DIV2,	CLK_PLL5,	2, 1),
	DEF_FIXED(".pll5_div4",		CLK_PLL5_DIV4,	CLK_PLL5_DIV2,	2, 1),
	DEF_FIXED(".pll6_div2",		CLK_PLL6_DIV2,	CLK_PLL6,	2, 1),
	DEF_FIXED(".s0",		CLK_S0,		CLK_PLL1_DIV2,	2, 1),
	DEF_FIXED(".sasyncper",		CLK_SASYNCPER,	CLK_PLL5_DIV4,  3, 1),
	DEF_FIXED(".sv_ir",		CLK_SV_IR,	CLK_PLL1,	5, 1),
	DEF_FIXED(".sv_vip",		CLK_SV_VIP,	CLK_PLL1,	5, 1),
	DEF_SDSCR(".sdsrc",		CLK_SDSRC,	CLK_PLL5, CPG_SD0CKCR1),
	DEF_RATE(".oco",		CLK_OCO,	32768),

	DEF_BASE(".rpcsrc",	CLK_RPCSRC, CLK_TYPE_R8A779G0_RPCSRC, CLK_PLL5),
	DEF_RPC("rpc",		R8A779G0_CLK_RPC,	CLK_RPCSRC, CPG_RPCCKCR),
	DEF_RPCD2("rpcd2",	R8A779G0_CLK_RPCD2,	R8A779G0_CLK_RPC),

	/* Core Clock Outputs */
	DEF_FIXED("s0d2",	R8A779G0_CLK_S0D2,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3",	R8A779G0_CLK_S0D3,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4",	R8A779G0_CLK_S0D4,	CLK_S0,		4, 1),
	DEF_FIXED("cl16m",	R8A779G0_CLK_CL16M,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_mm",	R8A779G0_CLK_S0D2_MM,	CLK_S0,		2, 1),
	DEF_FIXED("s0d4_mm",	R8A779G0_CLK_S0D4_MM,	CLK_S0,		4, 1),
	DEF_FIXED("cl16m_mm",	R8A779G0_CLK_CL16M_MM,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_rt",	R8A779G0_CLK_S0D2_RT,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3_rt",	R8A779G0_CLK_S0D3_RT,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4_rt",	R8A779G0_CLK_S0D4_RT,	CLK_S0,		4, 1),
	DEF_FIXED("s0d6_rt",	R8A779G0_CLK_S0D6_RT,	CLK_S0,		6, 1),
	DEF_FIXED("s0d24_rt",	R8A779G0_CLK_S0D24_RT,	CLK_S0,		24, 1),
	DEF_FIXED("cl16m_rt",	R8A779G0_CLK_CL16M_RT,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_per",	R8A779G0_CLK_S0D2_PER,	CLK_S0,		2, 1),
	DEF_FIXED("s0d3_per",	R8A779G0_CLK_S0D3_PER,	CLK_S0,		3, 1),
	DEF_FIXED("s0d4_per",	R8A779G0_CLK_S0D4_PER,	CLK_S0,		4, 1),
	DEF_FIXED("s0d6_per",	R8A779G0_CLK_S0D6_PER,	CLK_S0,		6, 1),
	DEF_FIXED("s0d12_per",	R8A779G0_CLK_S0D12_PER,	CLK_S0,		12, 1),
	DEF_FIXED("s0d24_per",	R8A779G0_CLK_S0D24_PER,	CLK_S0,		24, 1),
	DEF_FIXED("cl16m_per",	R8A779G0_CLK_CL16M_PER,	CLK_S0,		48, 1),
	DEF_FIXED("s0d1_hsc",	R8A779G0_CLK_S0D1_HSC,	CLK_S0,		1, 1),
	DEF_FIXED("s0d2_hsc",	R8A779G0_CLK_S0D2_HSC,	CLK_S0,		2, 1),
	DEF_FIXED("s0d4_hsc",	R8A779G0_CLK_S0D4_HSC,	CLK_S0,		4, 1),
	DEF_FIXED("s0d8_hsc",	R8A779G0_CLK_S0D8_HSC,	CLK_S0,		8, 1),
	DEF_FIXED("cl16m_hsc",	R8A779G0_CLK_CL16M_HSC,	CLK_S0,		48, 1),
	DEF_FIXED("s0d2_cc",	R8A779G0_CLK_S0D2_CC,	CLK_S0,		2, 1),
	DEF_FIXED("s0d1_vio",	R8A779G0_CLK_S0D1_VIO,	CLK_S0,		1, 1),
	DEF_FIXED("s0d2_vio",	R8A779G0_CLK_S0D2_VIO,	CLK_S0,		2, 1),
	DEF_FIXED("s0d4_vio",	R8A779G0_CLK_S0D4_VIO,	CLK_S0,		4, 1),
	DEF_FIXED("s0d8_vio",	R8A779G0_CLK_S0D8_VIO,	CLK_S0,		8, 1),
	DEF_FIXED("s0d1_vc",	R8A779G0_CLK_S0D1_VC,	CLK_S0,		1, 1),
	DEF_FIXED("s0d2_vc",	R8A779G0_CLK_S0D2_VC,	CLK_S0,		2, 1),
	DEF_FIXED("s0d4_vc",	R8A779G0_CLK_S0D4_VC,	CLK_S0,		4, 1),
	DEF_FIXED("s0d2_u3dg",	R8A779G0_CLK_S0D2_U3DG,	CLK_S0,		2, 1),
	DEF_FIXED("s0d4_u3dg",	R8A779G0_CLK_S0D4_U3DG,	CLK_S0,		4, 1),
	DEF_FIXED("svd1_vip",	R8A779G0_CLK_SVD1_VIP,	CLK_SV_VIP,	1, 1),
	DEF_FIXED("svd2_vip",	R8A779G0_CLK_SVD2_VIP,	CLK_SV_VIP,	2, 1),
	DEF_FIXED("svd1_ir",	R8A779G0_CLK_SVD1_IR,	CLK_SV_IR,	1, 1),
	DEF_FIXED("svd2_ir",	R8A779G0_CLK_SVD2_IR,	CLK_SV_IR,	2, 1),
	DEF_FIXED("cbfusa",	R8A779G0_CLK_CBFUSA,	CLK_EXTAL,	2, 1),
	DEF_FIXED("cp",		R8A779G0_CLK_CP,	CLK_EXTAL,	2, 1),
	DEF_FIXED("cpex",	R8A779G0_CLK_CPEX,	CLK_EXTAL,	2, 1),

	DEF_SD("sd0",		R8A779G0_CLK_SD0,	CLK_SDSRC, CPG_SD0CKCR0),

	DEF_DIV6P1("canfd",	R8A779G0_CLK_CANFD,	CLK_PLL5_DIV4, 0x0878),
	DEF_DIV6P1("mso",	R8A779G0_CLK_MSO,	CLK_PLL5_DIV4, CPG_MSOCKCR),
	DEF_DIV6P1("csi",	R8A779G0_CLK_CSI,	CLK_PLL5_DIV4, 0x0880),
	DEF_DIV6P1("dsiext",	R8A779G0_CLK_DSIEXT,	CLK_PLL5_DIV4, 0x0884),
	DEF_FIXED("dsiref",	R8A779G0_CLK_DSIREF,	CLK_PLL5_DIV4,	48, 1),
	DEF_FIXED("viobus",	R8A779G0_CLK_VIOBUS,	CLK_PLL5,	6, 1),
	DEF_FIXED("viobusd2",	R8A779G0_CLK_VIOBUSD2,	CLK_PLL5,	12, 1),
	DEF_FIXED("vcbus",	R8A779G0_CLK_VCBUS,	CLK_PLL5,	6, 1),
	DEF_FIXED("vcbusd2",	R8A779G0_CLK_VCBUSD2,	CLK_PLL5,	12, 1),
	DEF_FIXED("adgh",	R8A779G0_CLK_ADGH,	CLK_PLL5_DIV4,	1, 1),
	DEF_FIXED("impa1",	R8A779G0_CLK_IMPA,	CLK_PLL5_DIV4,	1, 1),
	DEF_FIXED("impa1d4",	R8A779G0_CLK_IMPA,	CLK_PLL5_DIV4,	4, 1),
	DEF_FIXED("sasyncrt",	R8A779G0_CLK_SASYNCRT,	CLK_PLL5_DIV4, 48, 1),
	DEF_FIXED("sasyncperd1", R8A779G0_CLK_SASYNCPERD1, CLK_SASYNCPER, 1, 1),
	DEF_FIXED("sasyncperd2", R8A779G0_CLK_SASYNCPERD2, CLK_SASYNCPER, 2, 1),
	DEF_FIXED("sasyncperd4", R8A779G0_CLK_SASYNCPERD4, CLK_SASYNCPER, 4, 1),

	DEF_OSC("osc",		R8A779G0_CLK_OSC,	CLK_EXTAL,	8),
	DEF_MDSEL("r",		R8A779G0_CLK_R, 29, CLK_EXTALR, 1, CLK_OCO, 1),
};

static const struct mssr_mod_clk r8a779g0_mod_clks[] = {
	DEF_MOD("adg",		122,	R8A779G0_CLK_ADGH),
	DEF_MOD("avb0",		211,	R8A779G0_CLK_S0D8_HSC),
	DEF_MOD("avb1",		212,	R8A779G0_CLK_S0D8_HSC),
	DEF_MOD("avb3",		213,	R8A779G0_CLK_S0D8_HSC),
	DEF_MOD("hscif0",	514,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("hscif1",	515,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("hscif2",	516,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("hscif3",	517,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("i2c0",		518,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c1",		519,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c2",		520,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c3",		521,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c4",		522,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("i2c5",		523,	R8A779G0_CLK_S0D6_PER),
	DEF_MOD("msi0",		618,	R8A779G0_CLK_MSO),
	DEF_MOD("msi1",		619,	R8A779G0_CLK_MSO),
	DEF_MOD("msi2",		620,	R8A779G0_CLK_MSO),
	DEF_MOD("msi3",		621,	R8A779G0_CLK_MSO),
	DEF_MOD("msi4",		624,	R8A779G0_CLK_MSO),
	DEF_MOD("msi5",		623,	R8A779G0_CLK_MSO),
	DEF_MOD("pcie0",	624,	R8A779G0_CLK_S0D2),
	DEF_MOD("pcie1",	625,	R8A779G0_CLK_S0D2),
	DEF_MOD("pwm",		628,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("rpc",		629,	R8A779G0_CLK_RPCD2),
	DEF_MOD("rtdm0",	630,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("rtdm1",	631,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("rtdm2",	700,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("rtdm3",	701,	R8A779G0_CLK_S0D2_RT),
	DEF_MOD("scif0",	702,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("scif1",	703,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("scif3",	704,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("scif4",	705,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("sdhi0",	706,	R8A779G0_CLK_SD0),
	DEF_MOD("sydm1",	709,	R8A779G0_CLK_S0D3_PER),
	DEF_MOD("sydm2",	710,	R8A779G0_CLK_S0D3_PER),
	DEF_MOD("tmu0",		713,	R8A779G0_CLK_SASYNCPERD1),
	DEF_MOD("tmu1",		714,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tmu2",		715,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tmu3",		716,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tmu4",		717,	R8A779G0_CLK_SASYNCPERD2),
	DEF_MOD("tpu",		718,	R8A779G0_CLK_SASYNCPERD4),
	DEF_MOD("wdt",		907,	R8A779G0_CLK_R),
	DEF_MOD("cmt0",		910,	R8A779G0_CLK_R),
	DEF_MOD("cmt1",		911,	R8A779G0_CLK_R),
	DEF_MOD("cmt2",		912,	R8A779G0_CLK_R),
	DEF_MOD("cmt3",		913,	R8A779G0_CLK_R),
	DEF_MOD("pfc0",		915,	R8A779G0_CLK_CL16M),
	DEF_MOD("pfc1",		916,	R8A779G0_CLK_CL16M),
	DEF_MOD("pfc2",		917,	R8A779G0_CLK_CL16M),
	DEF_MOD("pfc3",		918,	R8A779G0_CLK_CL16M),
	DEF_MOD("tsn",		2723,	R8A779G0_CLK_S0D4_HSC),
};

/*
 * CPG Clock Data
 */

/*
 * MD		EXTAL		OSC	PLL5
 * 14 13	(MHz)
 *-----------------------------------------
 * 0  0		16.66 x 1	/16	x192
 * 0  1		20.00 x 1	/19	x160
 * 1  0		Prohibited setting
 * 1  1		33.33 / 2	/32	x192
 */

#define CPG_PLL_CONFIG_INDEX(md)	((((md) & BIT(14)) >> 13) | \
					 (((md) & BIT(13)) >> 13))

static const struct rcar_gen4_cpg_pll_config cpg_pll_configs[4] = {
	/* EXTAL div	OSC prediv	PLL5 mult/div */
	{ 1,		16,		192,	1, },
	{ 1,		19,		160,	1, },
	{ 0,		0,		0,	0, },
	{ 2,		32,		192,	1, },
};

/*
 * Note that the only clock left running before booting Linux are now
 * MFIS, INTC-AP, INTC-EX, SCIF0, HSCIF0 on V4H
 */
#define MSTPCR5_HSCIF0	BIT(14) /* No information: MFIS, INTC-AP */
#define MSTPCR6_INTCEX	BIT(11) /* No information: MFIS, INTC-AP */
#define MSTPCR7_SCIF0	BIT(2) /* No information: MFIS, INTC-AP */
static const struct mstp_stop_table r8a779g0_mstp_table[] = {
	{ 0x0FC302A1, 0x0, 0x0, 0x0 },
	{ 0x00D50038, 0x0, 0x0, 0x0 },
	{ 0x00003800, 0x0, 0x0, 0x0 },
	{ 0xF0000000, 0x0, 0x0, 0x0 },
	{ 0x0001CE01, 0x0, 0x0, 0x0 },
	{ 0xEEFFE380, MSTPCR5_HSCIF0, 0x0, 0x0 },
	{ 0xF3FD3901, MSTPCR6_INTCEX, 0x0, 0x0 },
	{ 0xE007E6FF, MSTPCR7_SCIF0, 0x0, 0x0 },
	{ 0xC0003FFF, 0x0, 0x0, 0x0 },
	{ 0x001FBCF8, 0x0, 0x0, 0x0 },
	{ 0x30000000, 0x0, 0x0, 0x0 },
	{ 0x000000C3, 0x0, 0x0, 0x0 },
	{ 0xDE800000, 0x0, 0x0, 0x0 },
	{ 0x00000017, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x00000000, 0x0, 0x0, 0x0 },
	{ 0x000033C0, 0x0, 0x0, 0x0 },
	{ 0x402A001E, 0x0, 0x0, 0x0 },
	{ 0x0C010080, 0x0, 0x0, 0x0 },
};

static const void *r8a779g0_get_pll_config(const u32 cpg_mode)
{
	return &cpg_pll_configs[CPG_PLL_CONFIG_INDEX(cpg_mode)];
}

static const struct cpg_mssr_info r8a779g0_cpg_mssr_info = {
	.core_clk		= r8a779g0_core_clks,
	.core_clk_size		= ARRAY_SIZE(r8a779g0_core_clks),
	.mod_clk		= r8a779g0_mod_clks,
	.mod_clk_size		= ARRAY_SIZE(r8a779g0_mod_clks),
	.mstp_table		= r8a779g0_mstp_table,
	.mstp_table_size	= ARRAY_SIZE(r8a779g0_mstp_table),
	.reset_node		= "renesas,r8a779g0-rst",
	.reset_modemr_offset	= CPG_RST_MODEMR0,
	.reset_modemr1_offset	= CPG_RST_MODEMR1,
	.extalr_node		= "extalr",
	.mod_clk_base		= MOD_CLK_BASE,
	.clk_extal_id		= CLK_EXTAL,
	.clk_extalr_id		= CLK_EXTALR,
	.get_pll_config		= r8a779g0_get_pll_config,
	.reg_layout		= CLK_REG_LAYOUT_RCAR_V4H,
};

static const struct udevice_id r8a779g0_clk_ids[] = {
	{
		.compatible	= "renesas,r8a779g0-cpg-mssr",
		.data		= (ulong)&r8a779g0_cpg_mssr_info
	},
	{ }
};

U_BOOT_DRIVER(clk_r8a779g0) = {
	.name		= "clk_r8a779g0",
	.id		= UCLASS_CLK,
	.of_match	= r8a779g0_clk_ids,
	.priv_auto_alloc_size = sizeof(struct gen4_clk_priv),
	.ops		= &gen4_clk_ops,
	.probe		= gen4_clk_probe,
	.remove		= gen4_clk_remove,
};
