/*
 * arch/arm/cpu/armv8/cpu_info.c
 *	This file defines cpu-related functions.
 *
 * Copyright (C) 2015-2016 Renesas Electronics Corporation
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
	u32 product = rcar_get_cpu_type();
	u32 rev_integer = rcar_get_cpu_rev_integer();
	u32 rev_fraction = rcar_get_cpu_rev_fraction();

	switch (product) {
	case 0x4F:
		printf("CPU: Renesas Electronics R8A7795 rev %d.%d\n",
		       rev_integer, rev_fraction);
		if (strcmp(CONFIG_RCAR_TARGET_STRING, "r8a7795")) {
			printf("Warning: this code supports only %s\n",
			       CONFIG_RCAR_TARGET_STRING);
		}
		break;
	case 0x52:
		printf("CPU: Renesas Electronics R8A7796 rev %d.%d\n",
		       rev_integer, rev_fraction);
		if (strcmp(CONFIG_RCAR_TARGET_STRING, "r8a7796")) {
			printf("Warning: this code supports only %s\n",
			       CONFIG_RCAR_TARGET_STRING);
		}
		break;
	}
	return 0;
}
#endif /* CONFIG_DISPLAY_CPUINFO */
