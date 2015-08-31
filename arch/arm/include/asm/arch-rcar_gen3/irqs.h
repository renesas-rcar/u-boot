/*
 * arch/arm/include/asm/arch-rcar_gen3/irqs.h
 *	This file defines IRQ-related.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_MACH_IRQS_H
#define __ASM_MACH_IRQS_H

#define NR_IRQS         1024

/* GIC */
#define gic_spi(nr)		((nr) + 32)

/* INTCA */
#define evt2irq(evt)		(((evt) >> 5) - 16)
#define irq2evt(irq)		(((irq) + 16) << 5)

#endif /* __ASM_MACH_IRQS_H */
