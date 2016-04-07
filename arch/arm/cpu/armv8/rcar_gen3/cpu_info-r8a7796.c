/*
 * arch/arm/cpu/armv8/rcar_gen3/cpu_info-r8a7796.c
 *	This file defines cpu information funstions.
 *
 * Copyright (C) 2016 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>

#define PRR 0xFFF00044

u32 rcar_get_cpu_type(void)
{
	u32 product;

	product = readl(PRR);

	return (product & 0x00007F00) >> 8;
}

u32 rcar_get_cpu_rev_integer(void)
{
	u32 product;

	product = readl(PRR);

	return (u32)(((product & 0x000000F0) >> 4) + 1);
}

u32 rcar_get_cpu_rev_fraction(void)
{
	u32 product;

	product = readl(PRR);

	return (u32)(product & 0x0000000F);
}
