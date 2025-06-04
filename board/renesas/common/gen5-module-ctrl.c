// SPDX-License-Identifier: GPL-2.0-only
/*
 * gen5-module-ctrl.c
 *
 * Copyright (C) 2025 Renesas Electronics Corp.
 */

#include <asm/io.h>
#include <asm/mach-types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <renesas/gen5-clk-ctrl.h>

static u32 pdidMaxNums[28] = {
	[MOD_HIER_VIPN] =  1,
	[MOD_HIER_VIPS] =  1,
	[MOD_HIER_VIO]  =  7,
	[MOD_HIER_PERE] =  1,
	[MOD_HIER_PERW] =  0,
	[MOD_HIER_DDR0] =  0,
	[MOD_HIER_DDR1] =  0,
	[MOD_HIER_DDR2] =  0,
	[MOD_HIER_DDR3] =  0,
	[MOD_HIER_DDR4] =  0,
	[MOD_HIER_DDR5] =  0,
	[MOD_HIER_DDR6] =  0,
	[MOD_HIER_DDR7] =  0,
	[MOD_HIER_HSCN] =  6,
	[MOD_HIER_RT]   = 11,
	[MOD_HIER_TOP]  =  0,
	[MOD_HIER_HSCS] =  1,
	[MOD_HIER_IMN]  =  1,
	[MOD_HIER_IMS]  =  1,
	[MOD_HIER_GPC]  =  3,
	[MOD_HIER_DSP]  =  4,
	[MOD_HIER_MM]   =  0,
	[MOD_HIER_NPU0] = 13,
	[MOD_HIER_NPU1] = 13,
	[MOD_HIER_CMNN] = 20,
	[MOD_HIER_CMNS] = 19,
	[MOD_HIER_SCP]  =  0,
	[MOD_HIER_AON]  =  0,
};

static uintptr_t mdlc_returnBase(u32 mod_hier)
{
	switch (mod_hier) {
	case MOD_HIER_VIPN:
		return MDLC_BASE_VIPN;
	case MOD_HIER_VIPS:
		return MDLC_BASE_VIPS;
	case MOD_HIER_VIO:
		return MDLC_BASE_VIO;
	case MOD_HIER_PERE:
		return MDLC_BASE_PERE;
	case MOD_HIER_PERW:
		return MDLC_BASE_PERW;
	case MOD_HIER_DDR0:
		return MDLC_BASE_DDR0;
	case MOD_HIER_DDR1:
		return MDLC_BASE_DDR1;
	case MOD_HIER_DDR2:
		return MDLC_BASE_DDR2;
	case MOD_HIER_DDR3:
		return MDLC_BASE_DDR3;
	case MOD_HIER_DDR4:
		return MDLC_BASE_DDR4;
	case MOD_HIER_DDR5:
		return MDLC_BASE_DDR5;
	case MOD_HIER_DDR6:
		return MDLC_BASE_DDR6;
	case MOD_HIER_DDR7:
		return MDLC_BASE_DDR7;
	case MOD_HIER_HSCN:
		return MDLC_BASE_HSCN;
	case MOD_HIER_RT:
		return MDLC_BASE_RT;
	case MOD_HIER_TOP:
		return MDLC_BASE_TOP;
	case MOD_HIER_HSCS:
		return MDLC_BASE_HSCS;
	case MOD_HIER_IMN:
		return MDLC_BASE_IMN;
	case MOD_HIER_IMS:
		return MDLC_BASE_IMS;
	case MOD_HIER_GPC:
		return MDLC_BASE_GPC;
	case MOD_HIER_DSP:
		return MDLC_BASE_DSP;
	case MOD_HIER_MM:
		return MDLC_BASE_MM;
	case MOD_HIER_NPU0:
		return MDLC_BASE_NPU0;
	case MOD_HIER_NPU1:
		return MDLC_BASE_NPU1;
	case MOD_HIER_CMNN:
		return MDLC_BASE_CMNN;
	case MOD_HIER_CMNS:
		return MDLC_BASE_CMNS;
	case MOD_HIER_SCP:
		return MDLC_BASE_SCP;
	case MOD_HIER_AON:
		return MDLC_BASE_AON;
	default:
		log_debug("** Module Hierarchy %d is illegal.\n", mod_hier);
	}
	return 0;
}

static void mdlc_mpg_DisableWriteProtect(u32 mod_hier)
{
	/* Disable Write Protection */
	writel(MDLC_WRITE_EN, MDLC_PKCPROT0(mdlc_returnBase(mod_hier)));
}

static void mdlc_mpg_EnableWriteProtect(u32 mod_hier)
{
	/* Enable Write Protection */
	writel(MDLC_WRITE_DIS, MDLC_PKCPROT0(mdlc_returnBase(mod_hier)));
}

static void mdlc_ms_DisableWriteProtect(u32 mod_hier)
{
	/* Disable Write Protection */
	writel(MDLC_WRITE_EN, MDLC_PKCPROT1(mdlc_returnBase(mod_hier)));
}

static void mdlc_ms_EnableWriteProtect(u32 mod_hier)
{
	/* Enable Write Protection */
	writel(MDLC_WRITE_DIS, MDLC_PKCPROT1(mdlc_returnBase(mod_hier)));
}

static u32 mdlc_ms_checkStReg(uintptr_t base, u32 regNo)
{
	u32 ret = ERROR_END;
	u32 msress, msres;
	u32 timeout = 500000;

	while (--timeout) {
		msress = readl(MDLC_MSRESS(base, regNo));
		msres  = readl(MDLC_MSRES(base, regNo));
		if (msress == msres) {
			ret = NORMAL_END;
			break;
		}
		udelay(1000);
	}
	if (timeout == 0) {
		log_debug("** MS Status and Trigger are unmatched.\n");
		log_debug("MSRESS ( 0x%08lX )= 0x%08X\n", MDLC_MSRESS(base, regNo), msress);
		log_debug("MSRES  ( 0x%08lX )= 0x%08X\n", MDLC_MSRES(base, regNo), msres);
	}

	return ret;
}

static u32 mdlc_ms_ctrlStatus(u32 mod_hier, u32 regNo, u32 regBit, u32 mask)
{
	uintptr_t base = mdlc_returnBase(mod_hier);
	u32 msress;
	u32 val;

	/* Check current status. and if the transition does not need, skip following processes. */
	msress = readl(MDLC_MSRESS(base, regNo));
	if ((msress & mask) == regBit)
		return NORMAL_END;

	/* Check MSRES & MSRESS coherency */
	if (mdlc_ms_checkStReg(base, regNo) == ERROR_END) {
		log_debug("[NG] Current Status Error! [Hier=%d, RegNo=%d]\n", mod_hier, regNo);
		return ERROR_END;
	}

	/*------------------------------------------------------------------------*/
	mdlc_ms_DisableWriteProtect(mod_hier);

	/* Setting Module Standby */
	val = msress & (~mask);
	val |= regBit;
	writel(val, MDLC_MSRES(base, regNo));

	mdlc_ms_EnableWriteProtect(mod_hier);
	/*------------------------------------------------------------------------*/

	if (mdlc_ms_checkStReg(base, regNo) == ERROR_END) {
		log_debug("[NG] Transition is failed [Hier=%d, RegNo=%d, TargetBit=0x%x]\n",
		       mod_hier, regNo, regBit);
		return ERROR_END;
	}

	return NORMAL_END;
}

static u32 mdlc_ctrlModuleStandby(u32 mod_hier, u32 regNo, u32 bitpos, u32 status)
{
	uintptr_t base = mdlc_returnBase(mod_hier);
	u32 val, mask, timeout;
	u32 ret = NORMAL_END;

	/* Check Module Hierarchy */
	if (base == 0x0)
		return ERROR_END;

	val = status << bitpos;
	mask = 0x3 << bitpos;
	if (mdlc_ms_ctrlStatus(mod_hier, regNo, val, mask))
		ret = ERROR_END;

	/* Check Status */
	timeout = 10000;
	while (--timeout) {
		if ((readl(MDLC_MSRESS(base, regNo)) & mask) == val) {
			break;
		}
		udelay(1000);
	}
	if (timeout == 0) {
		log_debug("[NG] Transition is failed [Hier=%d, RegNo=%d, TargetBit=%d:%d]\n",
		       mod_hier, regNo, bitpos + 1, bitpos);
		log_debug("    MSRES  ( 0x%08lX )= 0x%08X\n", MDLC_MSRES(base, regNo),
			readl(MDLC_MSRES(base, regNo)));
		log_debug("        write-value = 0x%08X\n", val);
		ret = ERROR_END;
	}

	return ret;
}

static u32 mdlc_mpg_checkStReg(uintptr_t base, u32 pdid)
{
	u32 ret = ERROR_END;
	u32 mpdgs, mpdg;
	u32 timeout = 500000;

	while (--timeout) {
		mpdgs = readl(MDLC_MPDGS(base, pdid));
		mpdg = readl(MDLC_MPDG(base, pdid));
		if (mpdgs == mpdg) {
			ret = NORMAL_END;
			break;
		}
	}
	if (timeout == 0) {
		log_debug("** MPG Status and Trigger are unmatched.\n");
		log_debug("    MPDGS ( 0x%08lX )= 0x%08X\n", MDLC_MPDGS(base, pdid), mpdgs);
		log_debug("    MPDG  ( 0x%08lX )= 0x%08X\n", MDLC_MPDG(base, pdid), mpdg);
	}

	return ret;
}

static u32 mdlc_mpg_ctrlStatus(u32 mod_hier, u32 pdid)
{
	uintptr_t base = mdlc_returnBase(mod_hier);
	u32 curSt;
	u32 mpier, mpimr;

	/* Check current status. and if the transition does not need, skip following processes. */
	curSt = readl(MDLC_MPDGS(base, pdid));
	if (curSt == MPG_RUN)
		return NORMAL_END;

	/* Check MPDG & MPDGS coherency */
	if (mdlc_mpg_checkStReg(base, pdid) == ERROR_END) {
		log_debug("[NG] Current Status Error! [Hier=%d, PDID=%d]\n",
		       mod_hier, pdid);
		return ERROR_END;
	}

	/*------------------------------------------------------------------------*/
	mdlc_mpg_DisableWriteProtect(mod_hier);

	mpier = 0x00000000;
	mpimr = 0xFFFFFFFF;

	writel(mpier, MDLC_MPIER0(base));
	writel(mpimr, MDLC_MPIMR0(base));

	/* Transition to "Module Power Reset" */
	writel(MPG_RESET, MDLC_MPDG(base, pdid));
	if (mdlc_mpg_checkStReg(base, pdid) == ERROR_END) {
		mdlc_mpg_EnableWriteProtect(mod_hier);
		log_debug("[NG] Transition to \"Module Power Reset\" is failed [Hier=%d, PDID=%d]\n",
		       mod_hier, pdid);
		return ERROR_END;
	}

	writel(0x00000000, MDLC_MPIER0(base));
	writel(0xFFFFFFFF, MDLC_MPIMR0(base));

	/* Transition to "Module Power RUN" or "Module Power Gating" */
	writel(MPG_RUN, MDLC_MPDG(base, pdid));

	mdlc_mpg_EnableWriteProtect(mod_hier);
	/*------------------------------------------------------------------------*/

	if (mdlc_mpg_checkStReg(base, pdid) == ERROR_END) {
		log_debug("[NG] Transition to \"Module Power %s\" is failed [Hier=%d, PDID=%d]\n",
		       "RUN", mod_hier, pdid);
		return ERROR_END;
	}

	return NORMAL_END;
}

static u32 mdlc_ctrlModulePowerGating(u32 mod_hier)
{
	u32 pdid_max;
	s32 pdid, loop_lim;

	/* Check Module Hierarchy */
	if (mdlc_returnBase(mod_hier) == 0x0)
		return ERROR_END;

	/* Get Max value of each Module Hierarchy */
	pdid_max = pdidMaxNums[mod_hier];
	if (pdid_max == 0) {
		log_debug("** Module Number (%d) controlling is skip.\n", mod_hier);
		return NORMAL_END;
	}

	/* Controlling for each Power Domain in target module */
	pdid = pdid_max; /* PowerON : Control from highest to lowest PDIDs */
	loop_lim = 0;	 /* PowerOFF: Control from lowest to highest PDIDs */
	while (1) {
		/* Power Control */
		if (mdlc_mpg_ctrlStatus(mod_hier, pdid) == ERROR_END) {
			log_debug("** Module Number (%d) Power-%s is failure.\n", mod_hier, "ON");
			return ERROR_END;
		}

		/* Check if this PDID is the last element */
		if (pdid == loop_lim)
			break;

		pdid--;
	}

	return NORMAL_END;
}

void module_standby_set(u32 mod_hier, struct ms_info *p_ms)
{
	u32 msSequence;
	uintptr_t base = mdlc_returnBase(mod_hier);
	u32 ret = NORMAL_END;

	if (mdlc_ctrlModulePowerGating(mod_hier) != NORMAL_END) {
		log_debug("[NG] %s(), line: %d\n", __func__, __LINE__);
		ret = ERROR_END;
	}

	for (u32 i = 0;; i++) {
		/* Checking MS-table tail */
		if (p_ms[i].regNo == MDLC_TBL_END)
			break;

		log_debug("MDLC:  Clock-ON: Hier=%d, regNo=%d[%d:%d]\n", mod_hier,
		       p_ms[i].regNo, p_ms[i].regBit + 1, p_ms[i].regBit);

		/* Control clock */
		msSequence = MS_RESET;
		if (mdlc_ctrlModuleStandby(mod_hier,
					   p_ms[i].regNo,
					   p_ms[i].regBit,
					   msSequence) != NORMAL_END) {
			log_debug("[NG] %s(), line: %d\n", __func__, __LINE__);
			log_debug("     HIER:%d, regNo:%d, regBit:%d\n", mod_hier,
			       p_ms[i].regNo, p_ms[i].regBit);

			ret = ERROR_END;
		}
		log_debug("  MDLC%dMSRESS%d  ( 0x%08lX )= 0x%08X\n",
		       mod_hier,
		       p_ms[i].regNo,
		       MDLC_MSRESS(base, p_ms[i].regNo),
		       readl(MDLC_MSRESS(base, p_ms[i].regNo)));

		msSequence = MS_RUN;
		if (mdlc_ctrlModuleStandby(mod_hier,
					   p_ms[i].regNo,
					   p_ms[i].regBit,
					   msSequence) != NORMAL_END) {
			log_debug("[NG] %s(), line: %d\n", __func__, __LINE__);
			log_debug("     HIER:%d, regNo:%d, regBit:%d\n",
			       mod_hier,
			       p_ms[i].regNo,
			       p_ms[i].regBit);

			ret = ERROR_END;
		}
		log_debug("  MDLC%dMSRESS%d  ( 0x%08lX )= 0x%08X\n",
		       mod_hier,
		       p_ms[i].regNo,
		       MDLC_MSRESS(base, p_ms[i].regNo),
		       readl(MDLC_MSRESS(base, p_ms[i].regNo)));
	}
}

static void module_standby_pfc(void)
{
	uint32_t mod_hier = MOD_HIER_PERE;
	struct ms_info ms1[] = {
		{ 3,  0 },
		{ 3,  2 },
		{ 3,  4 },
		{ 3,  6 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */

	module_standby_set(mod_hier, ms1);

	mod_hier = MOD_HIER_PERW;
	struct ms_info ms2[] = {
		{ 3,  0 },
		{ 3,  2 },
		{ 3,  4 },
		{ 3,  6 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */

	module_standby_set(mod_hier, ms2);

	mod_hier = MOD_HIER_HSCN;
	struct ms_info ms3[] = {
		{ 5,  0 },
		{ 5,  2 },
		{ 5,  4 },
		{ 5,  6 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */

	module_standby_set(mod_hier, ms3);

	mod_hier = MOD_HIER_AON;
	struct ms_info ms4[] = {
		{ 2,  0 },
		{ 2,  2 },
		{ 2,  4 },
		{ 2,  6 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */

	module_standby_set(mod_hier, ms4);
}

static void module_standby_i2c(void)
{
	uint32_t mod_hier;
	mod_hier = MOD_HIER_PERW;
	struct ms_info ms1[] = {
		{ 4,  8 },
		{ 4,  10 },
		{ 4,  12 },
		{ 4,  14 },
		{ 4,  16 },
		{ 4,  18 },
		{ 4,  20 },
		{ 4,  22 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */
	module_standby_set(mod_hier, ms1);

	mod_hier = MOD_HIER_SCP;
	struct ms_info ms2[] = {
		{ 5,  8 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */
	module_standby_set(mod_hier, ms2);
}

void module_standby_pcs_rsw3_usb_mpphy(void)
{
	uint32_t mod_hier;
	mod_hier = MOD_HIER_HSCN;
	struct ms_info ms1[] = {
		{ 3,  0 },
		{ 3,  2 },
		{ 3,  4 },
		{ 3,  6 },
		{ 3,  8 },
		{ 3,  10 },
		{ 3,  12 },
		{ 3,  14 },
		{ 3,  16 },
		{ 3,  18 },
		{ 3,  20 },
		{ 3,  22 },
		{ 3,  24 },
		{ 3,  26 },
		{ 3,  28 },
		{ 3,  30 },
		{ 4,  0 },
		{ 4,  2 },
		{ 4,  4 },
		{ 4,  6 },
		{ 6,  0 },
		{ 6,  2 },
		{ 6,  4 },
		{ 6,  6 },
		{ 6,  8 },
		{ 6,  10 },
		{ 6,  12 },
		{ 6,  14 },
		{ 6,  16 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */
	module_standby_set(mod_hier, ms1);
}

static void module_standby_mmc(void)
{
	uint32_t mod_hier;
	mod_hier = MOD_HIER_PERE;
	struct ms_info ms1[] = {
		{ 7,  0 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */
	module_standby_set(mod_hier, ms1);
}

static void module_standby_pcie4(void)
{
	uint32_t mod_hier;
	mod_hier = MOD_HIER_HSCN;
	struct ms_info ms1[] = {
		{ 6,  18 },
		{ 6,  20 },
		{ 6,  22 },
		{ 6,  24 },
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */

	module_standby_set(mod_hier, ms1);
}

static void module_standby_ufs(void)
{
	uint32_t mod_hier;
	mod_hier = MOD_HIER_PERE;
	struct ms_info ms1[] = {
		{ 6,  0 },	/* UFS0 */
		{ 6,  2 },	/* UFS1 */
		{ MDLC_TBL_END, 0 },
	};      /* Target Registers on the hierarchy */
	module_standby_set(mod_hier, ms1);
}

void module_standby_wcrc(void)
{
	uint32_t mod_hier;
	mod_hier = MOD_HIER_RT;
	struct ms_info ms_wcrc[] = {
		/* wcrc0 to wcrc10 */
		{ 12, 26 },
		{ 12, 28 },
		{ 12, 30 },
		{ 13,  0 },
		{ 13,  2 },
		{ 13,  4 },
		{ 13,  6 },
		{ 13,  8 },
		{ 13, 10 },
		{ 13, 12 },
		{ 13, 14 },
		/* crc0 to crc10 */
		{ 13, 16 },
		{ 13, 18 },
		{ 13, 20 },
		{ 13, 22 },
		{ 13, 24 },
		{ 13, 26 },
		{ 13, 28 },
		{ 13, 30 },
		{ 14,  0 },
		{ 14,  2 },
		{ 14,  4 },
		/* kcrc0 to kcrc10 */
		{ 14,  6 },
		{ 14,  8 },
		{ 14, 10 },
		{ 14, 12 },
		{ 14, 14 },
		{ 14, 16 },
		{ 14, 18 },
		{ 14, 20 },
		{ 14, 22 },
		{ 14, 24 },
		{ 14, 26 },
		{ MDLC_TBL_END, 0 },
	};              /* Target Registers on the hierarchy */

	module_standby_set(mod_hier, ms_wcrc);
}


void module_standby_early_init(void)
{
	module_standby_pfc();
	module_standby_i2c();
}

void module_standby_init(void)
{
	module_standby_pcs_rsw3_usb_mpphy();
	module_standby_mmc();
	module_standby_pcie4();
	module_standby_ufs();
	module_standby_wcrc();
}
