// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/ebisu/ebisu.c
 *     This file is Ebisu board support.
 *
 * Copyright (C) 2018-2019 Renesas Electronics Corporation
 */

#include <common.h>
#include <malloc.h>
#include <netdev.h>
#include <dm.h>
#include <dm/platform_data/serial_sh.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/rmobile.h>
#include <asm/arch/rcar-mstp.h>
#include <asm/arch/sh_sdhi.h>
#include <i2c.h>
#include <mmc.h>
#include "../rcar-common/board_detect.h"

DECLARE_GLOBAL_DATA_PTR;

#define GPIO1_MSTP911		BIT(11)
#define GPIO3_MSTP909		BIT(9)
#define GPIO5_MSTP907		BIT(7)

void s_init(void)
{
}

int board_early_init_f(void)
{
	return 0;
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	return 0;
}

#define RST_BASE	0xE6160000
#define RST_CA57RESCNT	(RST_BASE + 0x40)
#define RST_CA53RESCNT	(RST_BASE + 0x44)
#define RST_RSTOUTCR	(RST_BASE + 0x58)
#define RST_CA57_CODE	0xA5A5000F
#define RST_CA53_CODE	0x5A5A000F

void reset_cpu(ulong addr)
{
	unsigned long midr, cputype;

	asm volatile("mrs %0, midr_el1" : "=r" (midr));
	cputype = (midr >> 4) & 0xfff;

	if (cputype == 0xd03)
		writel(RST_CA53_CODE, RST_CA53RESCNT);
	else if (cputype == 0xd07)
		writel(RST_CA57_CODE, RST_CA57RESCNT);
	else
		hang();
}

void board_add_ram_info(int use_default)
{
	int i;

	printf("\n");
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (!gd->bd->bi_dram[i].size)
			break;
		printf("Bank #%d: 0x%09llx - 0x%09llx, ", i,
		      (unsigned long long)(gd->bd->bi_dram[i].start),
		      (unsigned long long)(gd->bd->bi_dram[i].start
		      + gd->bd->bi_dram[i].size - 1));
		print_size(gd->bd->bi_dram[i].size, "\n");
	};
}

void board_cleanup_before_linux(void)
{
	/*
	 * Because of the control order dependency,
	 * turn off a specific clock at this timing
	 */
	mstp_setbits_le32(SMSTPCR9, SMSTPCR9,
			  GPIO1_MSTP911 | GPIO3_MSTP909 | GPIO5_MSTP907);
}

#if defined(CONFIG_MULTI_DTB_FIT)
int board_fit_config_name_match(const char *name)
{
	int ret;
	struct rcar_dram_conf_t dram_conf_addr;
	struct rcar_dt_fit_t dt_fit;

	dt_fit.board_id = 0xff;
	dt_fit.board_rev = 0xff;
	/*
	 * The name address register may be overwritten by the board_detect
	 * function. Backup it.
	 */
	dt_fit.target_name = name;

	ret = board_detect_type(&dt_fit);
	if (ret)
		return -1;

	ret = board_detect_dram(&dram_conf_addr);

	switch (dt_fit.board_id) {
	case BOARD_ID_EBISU:
		if (!strcmp(dt_fit.target_name, "r8a77990-ebisu-u-boot"))
			return 0;
		break;
	case BOARD_ID_EBISU_4D:
		if (!strcmp(dt_fit.target_name, "r8a77990-ebisu-4d-u-boot"))
			return 0;
		break;
	}

	return -1;
}
#endif
