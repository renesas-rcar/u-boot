/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * ./arch/arm/mach-rmobile/include/mach/rcar-gen5-base.h
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#ifndef __ASM_ARCH_RCAR_GEN5_BASE_H
#define __ASM_ARCH_RCAR_GEN5_BASE_H

/* Arm Generic Timer */
#define CNTCR_BASE		0xE6080000
#define CNTFID0		(CNTCR_BASE + 0x020)
#define CNTCR_EN		BIT(0)

/* GICv3 */
#define GICD_BASE	0x38000000

/* ReDistributor Registers for Control and Physical LPIs */
#define GICR_LPI_BASE	0x38060000
#define GICR_WAKER	0x0014
#define GICR_PWRR	0x0024
#define GICR_LPI_WAKER	(GICR_LPI_BASE + GICR_WAKER)
#define GICR_LPI_PWRR	(GICR_LPI_BASE + GICR_PWRR)

/* ReDistributor Registers for SGIs and PPIs */
#define GICR_SGI_BASE	0x38070000
#define GICR_IGROUPR0	0x0080

#ifndef __ASSEMBLY__
#include <asm/types.h>
#include <linux/bitops.h>
#endif

#endif /* __ASM_ARCH_RCAR_GEN5_BASE_H */
