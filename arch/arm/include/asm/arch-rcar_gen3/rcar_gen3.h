/*
 * arch/arm/include/asm/arch-rcar_gen3/rcar_gen3.h
 *	This file defines cpu-related function.
 *
 * Copyright (C) 2015-2016 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __ASM_ARCH_RCAR_GEN3_H
#define __ASM_ARCH_RCAR_GEN3_H

#if defined(CONFIG_RCAR_GEN3)
 #if defined(CONFIG_R8A7795)
 #include <asm/arch/r8a7795.h>
 #elif defined(CONFIG_R8A7796)
 #include <asm/arch/r8a7796.h>
 #else
 #error "SOC Name not defined"
 #endif
#else
 #error "not support ARCH."
#endif /* CONFIG_RCAR_GEN3 */

#ifndef __ASSEMBLY__
#include <asm/types.h>
u32 rcar_get_cpu_type(void);
u32 rcar_get_cpu_rev_integer(void);
u32 rcar_get_cpu_rev_fraction(void);
#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARCH_RCAR_GEN3_H */
