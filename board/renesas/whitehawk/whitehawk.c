// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/whitehawk/whitehawk.c
 *     This file is White Hawk board support.
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
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

void reset_cpu(ulong addr)
{
	writel(RST_SPRES, RST_SRESCR0);
}
