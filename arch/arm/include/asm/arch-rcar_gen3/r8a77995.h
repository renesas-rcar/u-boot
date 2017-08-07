/*
 * arch/arm/include/asm/arch-rcar_gen3/r8a77995.h
 *	This file defines registers and value for r8a77995.
 *
 * Copyright (C) 2017 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_R8A77995_H
#define __ASM_ARCH_R8A77995_H

#include "rcar-base.h"

/* Module stop control/status register bits */
#define MSTP0_BITS	0x00210000
#define MSTP1_BITS	0xFFFFFFFF
#define MSTP2_BITS	0x000E2FDC
#define MSTP3_BITS	0xFFFFFFDF
#define MSTP4_BITS	0x80000184
#define MSTP5_BITS	0xC3FFFFFF
#define MSTP6_BITS	0xFFFFFFFF
#define MSTP7_BITS	0xFFFFFFFF
#define MSTP8_BITS	0x00F1FFF7
#define MSTP9_BITS	0xF3F7FFD7
#define MSTP10_BITS	0xFFFEFFE0
#define MSTP11_BITS	0x000000B7

/* SDHI */
#define CONFIG_SYS_SH_SDHI2_BASE 0xEE140000	/* either MMC0 */
#define CONFIG_SYS_SH_SDHI_NR_CHANNEL 1

#define PUEN_USB0_OVC   (1 << 18)
#define PUEN_USB0_PWEN  (1 << 19)

#endif /* __ASM_ARCH_R8A77995_H */
