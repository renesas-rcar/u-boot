// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/falcon/falcon.c
 *     This file is Falcon board support.
 *
 * Copyright (C) 2020 Renesas Electronics Corp.
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/rmobile.h>

DECLARE_GLOBAL_DATA_PTR;

#define CPGWPR  0xE6150000
#define CPGWPCR 0xE6150004

#define	EXTAL_CLK	16666600u
#define CNTCR_BASE	0xE6080000
#define	CNTFID0		(CNTCR_BASE + 0x020)
#define CNTCR_EN	BIT(0)

void init_generic_timer(void)
{
	u32 freq, val;

	/* Set frequency data in CNTFID0 */
	freq = EXTAL_CLK;

	/* Update memory mapped and register based freqency */
	__asm__ volatile ("msr cntfrq_el0, %0" :: "r" (freq));
	writel(freq, CNTFID0);

	/* Enable counter */
	val = readl(CNTCR_BASE);
	writel(val | CNTCR_EN, CNTCR_BASE);
}

void s_init(void)
{
	init_generic_timer();
}

int board_early_init_f(void)
{
	/* Unlock CPG access */
	writel(0x5A5AFFFF, CPGWPR);
	writel(0xA5A50000, CPGWPCR);

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	return 0;
}

#define RST_BASE	0xE6160000 /* Domain0 */
#define RST_WDTRSTCR	(RST_BASE + 0x10)
#define RST_WDT_CODE	0xA55A0002

void reset_cpu(ulong addr)
{
	writel(RST_WDT_CODE, RST_WDTRSTCR);
}
