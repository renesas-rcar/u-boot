// SPDX-License-Identifier: GPL-2.0
/*
 * arch/arm/cpu/armv7/rmobile/cpu_info-rcar.c
 *
 * Copyright (C) 2013,2014 Renesas Electronics Corporation
 */
#include <common.h>
#include <asm/io.h>

#define PRR_MASK		0x7fff
#define R8A7796_REV_1_0		0x5200
#define R8A7796_REV_1_1		0x5210
#define R8A7796_REV_1_3		0x5211
#define R8A77995_REV_1_1	0x5810

static u32 rmobile_get_prr(void)
{
	if (IS_ENABLED(CONFIG_RCAR_64))
		return readl(0xFFF00044);
}

#define MIDR_PARTNUM_CORTEX_A720	0xD81
#define MIDR_PARTNUM_SHIFT		0x4
#define MIDR_PARTNUM_MASK		(0xFFF << 0x4)

static u32 read_midr(void)
{
	u32 val;

	asm volatile("mrs %0, midr_el1" : "=r" (val));

	return val;
}

#define is_cortex_a720() (((read_midr() & MIDR_PARTNUM_MASK) >>\
			 MIDR_PARTNUM_SHIFT) == MIDR_PARTNUM_CORTEX_A720)

u32 rmobile_get_cpu_type(void)
{
	if (IS_ENABLED(CONFIG_RCAR_GEN5) && is_cortex_a720())
		return RMOBILE_CPU_TYPE_R8A78000;

	return (rmobile_get_prr() & 0x00007F00) >> 8;
}

u32 rmobile_get_cpu_rev_integer(void)
{
	const u32 prr = rmobile_get_prr();
	const u32 rev = prr & PRR_MASK;

	if (rev == R8A7796_REV_1_1 || rev == R8A7796_REV_1_3 ||
	    rev == R8A77995_REV_1_1)
		return 1;
	else
		return ((prr & 0x000000F0) >> 4) + 1;
}

u32 rmobile_get_cpu_rev_fraction(void)
{
	const u32 prr = rmobile_get_prr();
	const u32 rev = prr & PRR_MASK;

	if (rev == R8A7796_REV_1_1 || rev == R8A77995_REV_1_1)
		return 1;
	else if (rev == R8A7796_REV_1_3)
		return 3;
	else
		return prr & 0x0000000F;
}
