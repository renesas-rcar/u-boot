// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/ebisu/ebisu.c
 *     This file is Ebisu board support.
 *
 * Copyright (C) 2018 Marek Vasut <marek.vasut+renesas@gmail.com>
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

int board_init(void)
{
	return 0;
}
