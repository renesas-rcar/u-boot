/*
 * arch/arm/include/asm/arch-rcar_gen3/r8a77995.h
 *	This file defines registers and value for r8a77995.
 *
 * Copyright (C) 2017-2018 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_R8A77995_H
#define __ASM_ARCH_R8A77995_H

#include "rcar-base.h"

/* SDHI */
#define CONFIG_SYS_SH_SDHI2_BASE 0xEE140000	/* either MMC0 */
#define CONFIG_SYS_SH_SDHI_NR_CHANNEL 1

#define PUEN_USB0_OVC   (1 << 18)
#define PUEN_USB0_PWEN  (1 << 19)

#endif /* __ASM_ARCH_R8A77995_H */
