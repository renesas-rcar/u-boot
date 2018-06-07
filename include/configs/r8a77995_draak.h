/*
 * include/configs/r8a77995_draak.h
 *     This file is draak board configuration.
 *     CPU r8a77995.
 *
 * Copyright (C) 2016-2018 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __DRAAK_H
#define __DRAAK_H

#undef DEBUG
#define CONFIG_RCAR_BOARD_STRING "Draak"
#define CONFIG_RCAR_TARGET_STRING "r8a77995"

#include "rcar-gen3-common.h"

/* SCIF */
#define CONFIG_SCIF_CONSOLE
#define CONFIG_CONS_SCIF2
#define CONFIG_SH_SCIF_CLK_FREQ             CONFIG_S3D4_CLK_FREQ
#define CONFIG_SH_SCIF_SSCG_CLK_FREQ        CONFIG_S3D4_SSCG_CLK_FREQ
#define CONFIG_SH_SCIF_CLEAN_CLK_FREQ       CONFIG_S3D4_CLEAN_CLK_FREQ

/* [A] Hyper Flash */
/* use to RPC(SPI Multi I/O Bus Controller) */

	/* underconstruction */

#define CONFIG_SYS_NO_FLASH

/* Ethernet RAVB */
#define CONFIG_RAVB											/* Enable the Ethernet driver */
#define CONFIG_RAVB_PHY_ADDR 0x0							/* Ethernet phy address */
#define CONFIG_RAVB_PHY_MODE PHY_INTERFACE_MODE_RGMII_ID	/* interface RGMIIÅiReduced Gigabit Media Independent InterfaceÅj*/
#define CONFIG_NET_MULTI									/* I do not know what to use */
#define CONFIG_PHYLIB										/* Enable all PHYs */
#define CONFIG_PHY_MICREL									/* Enable the MICREL phy */
#define CONFIG_BITBANGMII									/* Enable the miiphybb driver */
#define CONFIG_BITBANGMII_MULTI								/* Enable the multi bus support */
#define CONFIG_SH_ETHER_BITBANG								/* i do not know what I need */

/* Board Clock */
/* XTAL_CLK : 48.00MHz */
#define RCAR_XTAL_CLK					48000000u
#define CONFIG_SYS_CLK_FREQ				RCAR_XTAL_CLK
/* CPclk 24.00MHz, S3D2 133.33MHz , S3D4 66.66MHz          */
#define CONFIG_CP_CLK_FREQ				(CONFIG_SYS_CLK_FREQ / 2)		/* 24.00MHz) */
#define CONFIG_PLL1_CLK_FREQ			(CONFIG_SYS_CLK_FREQ * 100 / 3)	/* 1.6GHz) */
/* S3D2 SSCG OFF 133.33MHz  SSCG ON 125MHz */
#define CONFIG_S3D2_SSCG_CLK_FREQ		(266666666u/2)					/* 133.33MHz) */
#define CONFIG_S3D2_CLEAN_FREQ			(250000000u/2)					/* 125.00MHz) */
/* S3D4 SSCG OFF 66.66MHz  SSCG ON 62.5MHz */
#define CONFIG_S3D4_CLK_FREQ			(266666666u/4)					/* 66.66MHz) Dummy */
#define CONFIG_S3D4_CLEAN_CLK_FREQ		(266666666u/4)					/* 66.66MHz) */
#define CONFIG_S3D4_SSCG_CLK_FREQ		(250000000u/4)					/* 62.50MHz) */

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY				0x1800000			/* 24.0MHz from CPclk */

/* Generic Interrupt Controller Definitions */
#define GICD_BASE						(0xF1010000)		/* INTC-AP distributor */
#define GICC_BASE						(0xF1020000)		/* INTC-AP CPU Interfase */
#define CONFIG_GICV2

/* USB */
#define CONFIG_USB_STORAGE					/* Enable the USB storage devices */
#define CONFIG_USB_EHCI						/* Enable the generic EHCI driver */
#define CONFIG_USB_EHCI_RCAR_GEN3			/* Enable the rcar_gen3 EHCI driver */
#define CONFIG_USB_MAX_CONTROLLER_COUNT	1	/* USB contoroller max number */

/* SDHI */
#define CONFIG_MMC							/* MMC controller is enable */
#define CONFIG_CMD_MMC						/* MMC command line is enable */
#define CONFIG_GENERIC_MMC					/* Enable the generic MMC driver */
#define CONFIG_SH_SDHI_FREQ		200000000	/* SDHI MAX Frequency */
#define CONFIG_SH_SDHI_MMC					/* */


/* Environment in eMMC, at the end of 2nd "boot sector" */
#define CONFIG_ENV_IS_IN_MMC								/* Use MMC to save environment variables */
#define CONFIG_ENV_OFFSET               (-CONFIG_ENV_SIZE)	/* end of the MMC partition */
#define CONFIG_SYS_MMC_ENV_DEV          0					/* MMC device the environment is stored in */
#define CONFIG_SYS_MMC_ENV_PART         2					/* defaults to partition 0(user area) */

/* Module clock supply/stop status bits */
/* MFIS */
#define CONFIG_SMSTP2_ENA	0x00002000
/* serial(SCIF2) */
#define CONFIG_SMSTP3_ENA	0x00000400
/* INTC-AP, INTC-EX */
#define CONFIG_SMSTP4_ENA	0x00000180

/* ENV setting */
#define CONFIG_ENV_OVERWRITE							/* protection for vendor parameters is completely disabled */
#define CONFIG_ENV_SECT_SIZE    (128 * 1024)			/* Size of the sector containing the environment */
#define CONFIG_ENV_SIZE         (CONFIG_ENV_SECT_SIZE)  /* part of this flash sector for the environment */
#define CONFIG_ENV_SIZE_REDUND  (CONFIG_ENV_SIZE)		/* valid backup copy */

#define CONFIG_EXTRA_ENV_SETTINGS       \
	"fdt_high=0xffffffffffffffff\0" \
	"initrd_high=0xffffffffffffffff\0"

#define CONFIG_BOOTARGS \
	"root=/dev/nfs rw "     \
	"nfsroot=192.168.0.1:/export/rfs ip=192.168.0.20"

#define CONFIG_BOOTCOMMAND      \
	"tftp 0x48080000 Image; " \
	"tftp 0x48000000 Image-r8a77995-draak.dtb; " \
	"booti 0x48080000 - 0x48000000"

#endif /* __DRAAK_H */

