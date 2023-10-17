/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/rcar-gen5-common.h
 *	This file is R-Car Gen5 common configuration file.
 *
 * Copyright (C) 2023 Renesas Electronics Corporation
 */

#ifndef __RCAR_GEN5_COMMON_H
#define __RCAR_GEN5_COMMON_H

#include <asm/arch/rmobile.h>

#define CONFIG_REMAKE_ELF

/* boot option */

#define CONFIG_SYS_BOOTPARAMS_LEN	SZ_128K

/* Generic Interrupt Controller Definitions */
#define GICR_BASE	(GICR_LPI_BASE)

/* console */
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BAUDRATE_TABLE	{ 38400, 115200, 921600, 1843200 }

/* PHY needs a longer autoneg timeout */
#define PHY_ANEG_TIMEOUT		20000

/* MEMORY */
#define CONFIG_SYS_INIT_SP_ADDR		CONFIG_SYS_TEXT_BASE

#define DRAM_RSV_SIZE			0x08000000
#define CONFIG_SYS_SDRAM_BASE		(0x40000000 + DRAM_RSV_SIZE)
#define CONFIG_SYS_SDRAM_SIZE		(0x80000000u - DRAM_RSV_SIZE)
#define CONFIG_VERY_BIG_RAM
#define CONFIG_MAX_MEM_MAPPED		(0x80000000u - DRAM_RSV_SIZE)

#define CONFIG_SYS_MONITOR_BASE		0x00000000
#define CONFIG_SYS_MONITOR_LEN		(1 * 1024 * 1024)
#define CONFIG_SYS_BOOTM_LEN		(64 << 20)

/* The HF/QSPI layout permits up to 1 MiB large bootloader blob */
#define CONFIG_BOARD_SIZE_LIMIT		1048576

/* ENV setting */

#define CONFIG_EXTRA_ENV_SETTINGS	\
	"bootm_size=0x10000000\0"

#define CONFIG_BOOTCOMMAND	\
	"booti 0x48080000 - 0x48000000"

#endif	/* __RCAR_GEN5_COMMON_H */
