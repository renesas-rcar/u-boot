// SPDX-License-Identifier: GPL-2.0-only
/*
 * gen5-clk-ctrl.c
 *
 * Copyright (C) 2025 Renesas Electronics Corp.
 */

#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <renesas/gen5-clk-ctrl.h>

static const size_t pll_ctrl_reg2[PLL_MAX] = {
	/* Register address */
	PLL1_0_CR2,
	PLL1_1_CR2,
	PLL2_0_CR2,
	PLL2_1_CR2,
	PLL2_2_CR2,
	PLL2_3_CR2,
	PLL2_4_CR2,
	PLL2_5_CR2,
	PLL2_6_CR2,
	PLL2_7_CR2,
	PLL3_0_CR2,
	PLL3_1_CR2,
	PLL3_2_CR2,
	PLL3_3_CR2,
	PLL4_CR2,
	PLL5_CR2,
	PLL6_CR2,
	PLL7_CR2,
	PLL8_CR2,
	PLL9_0_CR2,
	PLL9_1_CR2,
	PLL10_CR2,
	PLL11_CR2,
	PLL12_CR2,
	PLL13_CR2,
	PLL14_CR2,
	PLL15_0_CR2,
	PLL15_1_CR2,
	PLL15_2_CR2,
	PLL15_3_CR2
};

static const size_t pll_sel_ctrl_reg[PLL_MAX][2U] = {
	/* Register address, Setting value */
	{PLL1_0SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL1_1SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_0SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_1SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_2SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_3SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_4SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_5SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_6SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL2_7SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL3_0SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL3_1SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL3_2SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL3_3SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL4SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL5SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL6SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL7SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL8SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL9_0SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL9_1SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL10SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL11SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL12SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL13SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL14SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL15_0SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL15_1SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL15_2SCR,	PLL_SCR_PLLSELID_CLK_PLL},
	{PLL15_3SCR,	PLL_SCR_PLLSELID_CLK_PLL}
};

static u32 get_pd_hier_from_pll(u32 pll_num)
{
	u32 pd_hier = 0xFFFFFFFFU;

	switch (pll_num) {
	case PLL1_0:
		pd_hier = PD_HIER_TOP;
		break;
	case PLL1_1:
		pd_hier = PD_HIER_TOP;
		break;
	case PLL5:
		pd_hier = PD_HIER_TOP;
		break;
	case PLL7:
		pd_hier = PD_HIER_TOP;
		break;
	case PLL12:
		pd_hier = PD_HIER_TOP;
		break;
	default:
		log_debug("PD_hier = Unknown PD_Hier error. Func:\n");
	}

	return pd_hier;
}

static void clock_controller_reg_write(size_t pd_hier, size_t reg_addr, size_t reg_val)
{
	/* Dummy read   */
	readl(reg_addr);

	switch (pd_hier) {
	case PD_HIER_PERE:
		writel(WRITE_KEY_CODE_EN, CLKPEREPKCPROT0);
		writel(reg_val, reg_addr);
		writel(WRITE_KEY_CODE_DIS, CLKPEREPKCPROT0);
		break;
	case PD_HIER_TOP:
		writel(WRITE_KEY_CODE_EN, CLKTOPPKCPROT0);
		writel(reg_val, reg_addr);
		writel(WRITE_KEY_CODE_DIS, CLKTOPPKCPROT0);
		break;
	case PD_HIER_HSCS:
		writel(WRITE_KEY_CODE_EN, CLKHSCSPKCPROT0);
		writel(reg_val, reg_addr);
		writel(WRITE_KEY_CODE_DIS, CLKHSCSPKCPROT0);
		break;
	case PD_HIER_SCP:
		writel(WRITE_KEY_CODE_EN, CLKSCPPKCPROT0);
		writel(reg_val, reg_addr);
		writel(WRITE_KEY_CODE_DIS, CLKSCPPKCPROT0);
		break;
	default:
		/* This PD Hier does not have Key Code Protection. */
		writel(reg_val, reg_addr);
		break;
	}

} /* End of function clock_controller_reg_write(u32 pd_hier, u32 reg_addr, u32 reg_val) */

static void pll_reg_write(u32 pll_num)
{
	u32 pd_hier = 0xFFFFFFFFU;

	pd_hier = get_pd_hier_from_pll(pll_num);

	clock_controller_reg_write(pd_hier, pll_sel_ctrl_reg[pll_num][0],
				   pll_sel_ctrl_reg[pll_num][1]);

} /* End of function pll_reg_write(u32 pll_num) */

u32 switch_clock_source_pll(u32 pll_num)
{
	/* Initial value of stable status is set to unstable. */
	u32 clk_stable_stat = PLL_CR2_PLLCLKSTAB_UNSTABLE;
	/* Initial value of clock source status is set to IOSC. */
	u32 clk_src_stat = PLL_SCR_PLLSELACT_CLK_IOSC;
	u32 cnt;
	u32 pd_hier;
	u32 pd_hier_impl_bit;
	u32 mx_info;

	// mx_info = read_register(PD_HIER_VR_PIN_ADDR);
	mx_info = readl(PD_HIER_VR_PIN_ADDR);
	pd_hier = get_pd_hier_from_pll(pll_num);
	pd_hier_impl_bit = BIT(0) << pd_hier;

	if ((mx_info & pd_hier_impl_bit) == PD_HIER_NOT_IMPL) {
		log_debug("PD_hier is not implemented.\n");
		return 0;
	}

	cnt = 0U;
	do {
		clk_stable_stat = PLL_CR2_PLLCLKSTAB_MASK &
				  readl((void __iomem *)pll_ctrl_reg2[pll_num]);
		cnt++;
		if (cnt > 1000) {
			log_debug("CPGM =  Give up waiting for clock stable state (PLLn_CR2.PLLCLKSTAB)\n");
			break;
		}
	} while (clk_stable_stat != PLL_CR2_PLLCLKSTAB_STABLE);

	/* 3. Change clock source to PLL. */
	pll_reg_write(pll_num);

	/* 4. Confirm that the clock source change is complete. */
	cnt = 0U;
	do {
		clk_src_stat = PLL_SCR_PLLSELACT_MASK &
			       readl((void __iomem *)pll_sel_ctrl_reg[pll_num][0]);
		cnt++;
		if (cnt > 1000) {
			log_debug(" Give up waiting for switching to PLL. (PLLnSCRegister.PLLnSELACT)\n");
			break;
		}
	} while (clk_src_stat != PLL_SCR_PLLSELACT_CLK_PLL);

	return 0;
}
