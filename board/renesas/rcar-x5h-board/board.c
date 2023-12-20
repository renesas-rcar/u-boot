// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/rcar-x5h/board.c
 *     This file is R-Car X5H board support.
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#include <common.h>
#include <asm/arch/rmobile.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/processor.h>
#include <linux/errno.h>
#include <asm/system.h>

DECLARE_GLOBAL_DATA_PTR;

#define EXTAL_CLK	16666666u

static void init_generic_timer(void)
{
	u32 freq;

	/* Set frequency data in CNTFID0 */
	freq = EXTAL_CLK;

	/* Update memory mapped and register based freqency */
	asm volatile ("msr cntfrq_el0, %0" :: "r" (freq));
	writel(freq, CNTFID0);

	/* Enable counter */
	setbits_le32(CNTCR_BASE, CNTCR_EN);
}

void s_init(void)
{
	if (current_el() == 3)
		init_generic_timer();
}

int board_early_init_f(void)
{
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	return 0;
}

void reset_cpu(void)
{
	return ;
}
