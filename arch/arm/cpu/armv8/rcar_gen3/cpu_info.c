/*
 * arch/arm/cpu/armv8/cpu_info.c
 *	This file defines cpu-related functions.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>

#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	icache_enable();
	return 0;
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	dcache_enable();
}
#endif

#ifdef CONFIG_DISPLAY_CPUINFO
static u32 __rcar_get_cpu_type(void)
{
	return 0x0;
}
u32 rcar_get_cpu_type(void)
		__attribute__((weak, alias("__rcar_get_cpu_type")));

static u32 __rcar_get_cpu_rev_integer(void)
{
	return 0;
}
u32 rcar_get_cpu_rev_integer(void)
		__attribute__((weak, alias("__rcar_get_cpu_rev_integer")));

static u32 __rcar_get_cpu_rev_fraction(void)
{
	return 0;
}
u32 rcar_get_cpu_rev_fraction(void)
		__attribute__((weak, alias("__rcar_get_cpu_rev_fraction")));

int print_cpuinfo(void)
{
	/* rcar_get_cpu_type() */
	printf("CPU: Renesas Electronics CPU rev %d.%d\n",
	       rcar_get_cpu_rev_integer(),
		rcar_get_cpu_rev_fraction());
	return 0;
}
#endif /* CONFIG_DISPLAY_CPUINFO */
