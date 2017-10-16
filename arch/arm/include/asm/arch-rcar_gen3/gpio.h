/*
 * arch/arm/include/asm/arch-rcar_gen3/gpio.h
 *	This file defines gpio-related function.
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#include <common.h>
#if defined(CONFIG_R8A7795)
#include <asm/arch/r8a7795_es-gpio.h>
#include <asm/arch/r8a7795-gpio.h>
#elif defined(CONFIG_R8A7796X)
#include <asm/arch/r8a7796-gpio.h>
#endif

#if defined(CONFIG_R8A7795)
void r8a7795_pinmux_init(void);
void r8a7795_es_pinmux_init(void);
#elif defined(CONFIG_R8A7796X)
void r8a7796_pinmux_init(void);
#endif
void pinmux_init(void);

#endif /* __ASM_ARCH_GPIO_H */
