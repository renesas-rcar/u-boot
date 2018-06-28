/*
 * arch/arm/include/asm/arch-rcar_gen3/r8a7796.h
 *	This file defines registers and value for r8a7796.
 *
 * Copyright (C) 2016-2018 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_R8A7796_H
#define __ASM_ARCH_R8A7796_H

#include "rcar-base.h"

/* SDHI */
#define CONFIG_SYS_SH_SDHI0_BASE 0xEE100000
#define CONFIG_SYS_SH_SDHI1_BASE 0xEE120000
#define CONFIG_SYS_SH_SDHI2_BASE 0xEE140000	/* either MMC0 */
#define CONFIG_SYS_SH_SDHI3_BASE 0xEE160000	/* either MMC1 */
#define CONFIG_SYS_SH_SDHI_NR_CHANNEL 4

#endif /* __ASM_ARCH_R8A7795_H */
