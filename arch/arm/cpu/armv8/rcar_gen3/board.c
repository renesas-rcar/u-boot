/*
 * arch/arm/cpu/armv8/rcar_gen3/board.c
 *	This file defines board-related function.
 *
 * Copyright 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>

int checkboard(void)
{
	printf("Board: %s\n", sysinfo.board_string);
	return 0;
}
