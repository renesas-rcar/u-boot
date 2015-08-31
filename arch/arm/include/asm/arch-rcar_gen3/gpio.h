/*
 * arch/arm/include/asm/arch-rcar_gen3/gpio.h
 *	This file defines gpio-related function.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#if defined(CONFIG_R8A7795)
#include "r8a7795-gpio.h"
void r8a7795_pinmux_init(void);
#endif

#endif /* __ASM_ARCH_GPIO_H */
