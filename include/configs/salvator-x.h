/*
 * include/configs/salvator-x.h
 *     This file is Salvator-X board configuration.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __SALVATOR_X_H
#define __SALVATOR_X_H

#undef DEBUG
#define CONFIG_R8A7795
#define CONFIG_R8A7795_WS
#define CONFIG_RCAR_BOARD_STRING "Salvator-X"

#include "rcar-gen3-common.h"

/* Cache Definitions */
#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_SYS_ICACHE_OFF

/* SCIF */
#define CONFIG_SCIF_CONSOLE
#define CONFIG_CONS_SCIF2
#define CONFIG_CONS_INDEX	2
#define CONFIG_SH_SCIF_CLK_FREQ	CONFIG_SYS_CLK_FREQ

/* [A] Hyper Flash */
/* use to RPC(SPI Multi I/O Bus Controller) */

	/* underconstruction */

#define CONFIG_SYS_NO_FLASH

/* Ethernet RAVB */
#define CONFIG_RAVB
#define CONFIG_RAVB_PHY_ADDR 0x0
#define CONFIG_RAVB_PHY_MODE PHY_INTERFACE_MODE_GMII
#define CONFIG_NET_MULTI
#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL
#define CONFIG_BITBANGMII
#define CONFIG_BITBANGMII_MULTI
#define CONFIG_SH_ETHER_BITBANG

/* Board Clock */
/* XTAL_CLK : 33.33MHz */
#define RCAR_XTAL_CLK	33333333u
#define CONFIG_SYS_CLK_FREQ	RCAR_XTAL_CLK
/* ch0to2 CPclk, ch3to11 S3D2_PEREclk, ch12to14 S3D2_RTclk */
/* CPclk 16.66MHz, S3D2 133.33MHz                          */
#define CONFIG_CP_CLK_FREQ	(CONFIG_SYS_CLK_FREQ / 2)
#define CONFIG_PLL1_CLK_FREQ	(CONFIG_SYS_CLK_FREQ * 192 / 2)
#define CONFIG_S3D2_CLK_FREQ	(266666666u/2)

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0xFE502A	/* 16.66MHz from CPclk */

/* Generic Interrupt Controller Definitions */
#define GICD_BASE	(0xF1010000)
#define GICC_BASE	(0xF1020000)
#define CONFIG_GICV2

/* USB */
#define CONFIG_USB_STORAGE
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_RCAR_GEN3
#define CONFIG_USB_MAX_CONTROLLER_COUNT	3

/* Module stop status bits */
/* MFIS, SCIF1 */
#define CONFIG_SMSTP2_ENA	0x00002040
/* INTC-AP, IRQC */
#define CONFIG_SMSTP4_ENA	0x00000180

/* SDHI */
#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_SH_SDHI_FREQ	CLKDEV_HS_DATA
#define CONFIG_SH_SDHI_MMC

/* Environment in eMMC, at the end of 2nd "boot sector" */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_ENV_OFFSET               (-CONFIG_ENV_SIZE)
#define CONFIG_SYS_MMC_ENV_DEV          1
#define CONFIG_SYS_MMC_ENV_PART         2

#endif /* __SALVATOR_X_H */
