// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/eagle/eagle.c
 *     This file is Eagle board support.
 *
 * Copyright (C) 2017 Marek Vasut <marek.vasut+renesas@gmail.com>
 */

#include <common.h>
#include <cpu_func.h>
#include <hang.h>
#include <init.h>
#include <malloc.h>
#include <netdev.h>
#include <dm.h>
#include <asm/global_data.h>
#include <dm/platform_data/serial_sh.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/rmobile.h>
#include <asm/arch/rcar-mstp.h>
#include <asm/arch/sh_sdhi.h>
#include <i2c.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#define CPGWPR  0xE6150900
#define CPGWPCR	0xE6150904

/* PLL */
#define PLL0CR		0xE61500D8
#define PLL0_STC_MASK	0x7F000000
#define PLL0_STC_OFFSET	24

#define CLK2MHZ(clk)	(clk / 1000 / 1000)
void s_init(void)
{
	struct rcar_rwdt *rwdt = (struct rcar_rwdt *)RWDT_BASE;
	struct rcar_swdt *swdt = (struct rcar_swdt *)SWDT_BASE;
	u32 stc;

	/* Watchdog init */
	writel(0xA5A5A500, &rwdt->rwtcsra);
	writel(0xA5A5A500, &swdt->swtcsra);

	/* CPU frequency setting. Set to 0.8GHz */
	stc = ((800 / CLK2MHZ(CONFIG_SYS_CLK_FREQ)) - 1) << PLL0_STC_OFFSET;
	clrsetbits_le32(PLL0CR, PLL0_STC_MASK, stc);
}

int board_early_init_f(void)
{
	/* Unlock CPG access */
	writel(0xA5A5FFFF, CPGWPR);
	writel(0x5A5A0000, CPGWPCR);

	return 0;
}

int board_init(void)
{
	return 0;
}
