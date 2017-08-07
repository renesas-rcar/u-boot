/*
 * arch/arm/cpu/armv8/rcar_gen3/pfc.c
 *     This file is r8a7795/r8a7796/r8a77995 processor support
 *       - PFC hardware block.
 *
 * Copyright (C) 2016-2017 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/arch/prr_depend.h>

void pinmux_init(void)
{
#if defined(CONFIG_R8A7795)
	if (rcar_is_legacy())
		r8a7795_es_pinmux_init();
	else
		r8a7795_pinmux_init();
#elif defined(CONFIG_R8A7796X)
	r8a7796_pinmux_init();
#elif defined(CONFIG_R8A77995)
	r8a77995_pinmux_init();
#endif
}
