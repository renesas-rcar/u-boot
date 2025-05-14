/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * ./arch/arm/mach-rmobile/include/mach/rcar-gen4-base.h
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 */

#ifndef __ASM_ARCH_RCAR_GEN4_BASE_H
#define __ASM_ARCH_RCAR_GEN4_BASE_H

/*
 * R-Car (R8A779F0) I/O Addresses
 */
#define RWDT_BASE		ADDR_ASSIGN_RGID(0xE6020000, CONFIG_RCAR_RGID)
#define SWDT_BASE		ADDR_ASSIGN_RGID(0xE6030000, CONFIG_RCAR_RGID)
#define TMU_BASE		ADDR_ASSIGN_RGID(0xE61E0000, CONFIG_RCAR_RGID)

/* SCIF */
#define SCIF0_BASE		ADDR_ASSIGN_RGID(0xE6E60000, CONFIG_RCAR_RGID)
#define SCIF1_BASE		ADDR_ASSIGN_RGID(0xE6E68000, CONFIG_RCAR_RGID)
#define SCIF2_BASE		ADDR_ASSIGN_RGID(0xE6E88000, CONFIG_RCAR_RGID)
#define SCIF3_BASE		ADDR_ASSIGN_RGID(0xE6C50000, CONFIG_RCAR_RGID)
#define SCIF4_BASE		ADDR_ASSIGN_RGID(0xE6C40000, CONFIG_RCAR_RGID)
#define SCIF5_BASE		ADDR_ASSIGN_RGID(0xE6F30000, CONFIG_RCAR_RGID)

/* CPG */
#define CPGWPR			ADDR_ASSIGN_RGID(0xE6158000, CONFIG_RCAR_RGID)
#define CPGWPCR			ADDR_ASSIGN_RGID(0xE6158004, CONFIG_RCAR_RGID)

/* Reset */
#define RST_BASE		ADDR_ASSIGN_RGID(0xE6168000, CONFIG_RCAR_RGID) /* Domain2 */
#define RST_SRESCR0		(RST_BASE + 0x18)
#define RST_SPRES		0x5AA58000

/* Arm Generic Timer */
#define CNTCR_BASE		ADDR_ASSIGN_RGID(0xE6080000, CONFIG_RCAR_RGID)
#define CNTFID0			(CNTCR_BASE + 0x020)
#define CNTCR_EN		BIT(0)

/* GICv3 */
/* Distributor Registers */
#define GICD_BASE	ADDR_ASSIGN_RGID(0xF1000000, CONFIG_RCAR_RGID)

/* ReDistributor Registers for Control and Physical LPIs */
#define GICR_LPI_BASE	ADDR_ASSIGN_RGID(0xF1060000, CONFIG_RCAR_RGID)
#define GICR_WAKER	0x0014
#define GICR_PWRR	0x0024
#define GICR_LPI_WAKER	(GICR_LPI_BASE + GICR_WAKER)
#define GICR_LPI_PWRR	(GICR_LPI_BASE + GICR_PWRR)

/* ReDistributor Registers for SGIs and PPIs */
#define GICR_SGI_BASE	ADDR_ASSIGN_RGID(0xF1070000, CONFIG_RCAR_RGID)
#define GICR_IGROUPR0	0x0080

#ifndef __ASSEMBLY__
#include <asm/types.h>
#include <linux/bitops.h>

/* RWDT */
struct rcar_rwdt {
	u32 rwtcnt;
	u32 rwtcsra;
	u32 rwtcsrb;
};

/* SWDT */
struct rcar_swdt {
	u32 swtcnt;
	u32 swtcsra;
	u32 swtcsrb;
};
#endif

#endif /* __ASM_ARCH_RCAR_GEN4_BASE_H */
