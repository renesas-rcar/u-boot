/*
 * arch/arm/cpu/armv8/rcar_gen3/emac.c
 *	This file defines ethernet initialize functions.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <netdev.h>

int cpu_eth_init(bd_t *bis)
{
	int ret = -ENODEV;
#ifdef CONFIG_SH_ETHER
	ret = sh_eth_initialize(bis);
#endif
	return ret;
}
