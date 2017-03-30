/*
 * include/configs/rcar-gen3-common.h
 *	This file is R-Car Gen3 common configuration file.
 *
 * Copyright (C) 2015-2016 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __RCAR_GEN3_COMMON_H
#define __RCAR_GEN3_COMMON_H

#include <asm/arch/rcar_gen3.h>
#include <config_distro_defaults.h>

/* Flash not used from u-boot */
#define CONFIG_SYS_NO_FLASH
#include <config_cmd_default.h>

#define CONFIG_CMD_DFL
#define CONFIG_CMD_SDRAM
#define CONFIG_CMD_USB
#define CONFIG_CMD_EXT4_WRITE

#define CONFIG_SYS_THUMB_BUILD
#define CONFIG_SYS_GENERIC_BOARD

#define CONFIG_REMAKE_ELF

/* Support File sytems */
#define CONFIG_FAT_WRITE
#define CONFIG_EXT4_WRITE

#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG

#define CONFIG_BAUDRATE		115200

#define CONFIG_VERSION_VARIABLE
#undef	CONFIG_SHOW_BOOT_PROGRESS

#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO
#define CONFIG_BOARD_EARLY_INIT_F

#define CONFIG_TMU_TIMER
#define CONFIG_SH_GPIO_PFC

/* console */
#undef  CONFIG_SYS_CONSOLE_INFO_QUIET
#undef  CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
#undef  CONFIG_SYS_CONSOLE_ENV_OVERWRITE

#define CONFIG_SYS_CBSIZE		256
#define CONFIG_SYS_PBSIZE		256
#define CONFIG_SYS_MAXARGS		16
#define CONFIG_SYS_BARGSIZE		512
#define CONFIG_SYS_BAUDRATE_TABLE	{ 115200, 38400 }

/* MEMORY */
#define CONFIG_SYS_TEXT_BASE		0x50000000
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_TEXT_BASE + 0x7fff0)


#define DRAM_RSV_SIZE			0x08000000
#if defined(CONFIG_R8A7795)
#define CONFIG_NR_DRAM_BANKS		4
#define PHYS_SDRAM_1			(0x40000000 + DRAM_RSV_SIZE)	/* legacy */
#define PHYS_SDRAM_1_SIZE		((unsigned long)(0x40000000 - DRAM_RSV_SIZE))
#define PHYS_SDRAM_2			0x0500000000		/* ext */
#define PHYS_SDRAM_2_SIZE		((unsigned long)0x40000000)
#define PHYS_SDRAM_3			0x0600000000		/* ext */
#define PHYS_SDRAM_3_SIZE		((unsigned long)0x40000000)
#define PHYS_SDRAM_4			0x0700000000		/* ext */
#define PHYS_SDRAM_4_SIZE		((unsigned long)0x40000000)
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_SDRAM_SIZE		PHYS_SDRAM_1_SIZE
#elif defined(CONFIG_R8A7796)
#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_1			(0x40000000 + DRAM_RSV_SIZE)	/* legacy */
#define PHYS_SDRAM_1_SIZE		((unsigned long)(0x80000000 - DRAM_RSV_SIZE))
#define PHYS_SDRAM_2			0x0600000000		/* ext */
#define PHYS_SDRAM_2_SIZE		((unsigned long)0x80000000)
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_SDRAM_SIZE		PHYS_SDRAM_1_SIZE
#else
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_SDRAM_SIZE		(1024u * 1024 * 1024)
#endif
#define CONFIG_SYS_LOAD_ADDR		(0x48080000)
#define CONFIG_VERY_BIG_RAM
#define CONFIG_MAX_MEM_MAPPED		CONFIG_SYS_SDRAM_SIZE

#define CONFIG_SYS_MONITOR_BASE		0x00000000
#define CONFIG_SYS_MONITOR_LEN		(256 * 1024)
#define CONFIG_SYS_MALLOC_LEN		(1 * 1024 * 1024)
#define CONFIG_SYS_BOOTMAPSZ		(8 * 1024 * 1024)

#define ENV_MEM_LAYOUT_SETTINGS \
	"fdt_high=0xffffffffffffffff\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"fdt_addr_r=0x48000000\0" \
	"pxefile_addr_r=0x48000000\0" \
	"kernel_addr_r=0x48080000\0" \
	"scriptaddr=0x49000000\0" \
	"ramdisk_addr_r=0x49100000\0" \

/* Default boot targets, SD first, then eMMC, USB and network */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(USB, usb, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

#endif	/* __RCAR_GEN3_COMMON_H */
