// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/salvator-x/salvator-x.c
 *     This file is Salvator-X/Salvator-XS board support.
 *
 * Copyright (C) 2015-2019 Renesas Electronics Corporation
 * Copyright (C) 2015 Nobuhiro Iwamatsu <iwamatsu@nigauri.org>
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

void s_init(void)
{
}

#define DVFS_MSTP926		BIT(26)
#define GPIO2_MSTP910		BIT(10)
#define GPIO3_MSTP909		BIT(9)
#define GPIO5_MSTP907		BIT(7)
#define HSUSB_MSTP704		BIT(4)	/* HSUSB */

int board_early_init_f(void)
{
#if defined(CONFIG_SYS_I2C) && defined(CONFIG_SYS_I2C_SH)
	/* DVFS for reset */
	mstp_clrbits_le32(SMSTPCR9, SMSTPCR9, DVFS_MSTP926);
#endif
	return 0;
}

/* HSUSB block registers */
#define HSUSB_REG_LPSTS			0xE6590102
#define HSUSB_REG_LPSTS_SUSPM_NORMAL	BIT(14)
#define HSUSB_REG_UGCTRL2		0xE6590184
#define HSUSB_REG_UGCTRL2_USB0SEL	0x30
#define HSUSB_REG_UGCTRL2_USB0SEL_EHCI	0x10

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	/* USB1 pull-up */
	setbits_le32(PFC_PUEN6, PUEN_USB1_OVC | PUEN_USB1_PWEN);

	/* Configure the HSUSB block */
	mstp_clrbits_le32(SMSTPCR7, SMSTPCR7, HSUSB_MSTP704);
	/* Choice USB0SEL */
	clrsetbits_le32(HSUSB_REG_UGCTRL2, HSUSB_REG_UGCTRL2_USB0SEL,
			HSUSB_REG_UGCTRL2_USB0SEL_EHCI);
	/* low power status */
	setbits_le16(HSUSB_REG_LPSTS, HSUSB_REG_LPSTS_SUSPM_NORMAL);

	return 0;
}

#define RST_BASE	0xE6160000
#define RST_CA57RESCNT	(RST_BASE + 0x40)
#define RST_CA53RESCNT	(RST_BASE + 0x44)
#define RST_RSTOUTCR	(RST_BASE + 0x58)
#define RST_CODE	0xA5A5000F

void reset_cpu(ulong addr)
{
#if defined(CONFIG_SYS_I2C) && defined(CONFIG_SYS_I2C_SH)
	i2c_reg_write(CONFIG_SYS_I2C_POWERIC_ADDR, 0x20, 0x80);
#else
	/* only CA57 ? */
	writel(RST_CODE, RST_CA57RESCNT);
#endif
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
	}
}

void board_cleanup_before_linux(void)
{
	/*
	 * Turn off the clock that was turned on outside
	 * the control of the driver
	 */
	/* Configure the HSUSB block */
	mstp_setbits_le32(SMSTPCR7, SMSTPCR7, HSUSB_MSTP704);

	/*
	 * Because of the control order dependency,
	 * turn off a specific clock at this timing
	 */
	mstp_setbits_le32(SMSTPCR9, SMSTPCR9,
			  GPIO2_MSTP910 | GPIO3_MSTP909 | GPIO5_MSTP907);
}

#if defined(CONFIG_MULTI_DTB_FIT)
int board_fit_config_name_match(const char *name)
{
	int ret;
#if defined(CONFIG_R8A7795) || defined(CONFIG_R8A7796)
	int i;
	int bank_num;
	u64 bank_size;
#endif
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

	switch (dt_fit.board_id) {	 /* only supported board */
	case BOARD_ID_SALVATOR_X:
#if defined(CONFIG_R8A7795)
		if (!strcmp(dt_fit.target_name, "r8a7795-salvator-x-u-boot"))
			return 0;
#endif
#if defined(CONFIG_R8A7796)
		if (!strcmp(dt_fit.target_name, "r8a7796-salvator-x-u-boot"))
			return 0;
#endif
#if defined(CONFIG_R8A77965)
		if (!strcmp(dt_fit.target_name, "r8a77965-salvator-x-u-boot"))
			return 0;
#endif
		break;
	case BOARD_ID_SALVATOR_XS:
#if defined(CONFIG_R8A7795)
		if (!ret) {
			/* select memory type */
			bank_num = 0;
			for (i = 0; i < 4; i++) {
				if (dram_conf_addr.address[i])
					bank_num++;
				else
					break;
			};
			bank_size = dram_conf_addr.size[0];
			if (!strcmp(dt_fit.target_name,
				    "r8a7795-salvator-xs-u-boot") &&
			    bank_num == 4 && bank_size == 0x40000000) {
				return 0;
			} else if (!strcmp(dt_fit.target_name,
					   "r8a7795-salvator-xs-4x2g-u-boot") &&
				   bank_num == 4 && bank_size == 0x80000000) {
				return 0;
			} else if (!strcmp(dt_fit.target_name,
					   "r8a7795-salvator-xs-2x2g-u-boot") &&
				   bank_num == 2 && bank_size == 0x80000000) {
				return 0;
			}
		}
		/* else works default : return -1 */
#endif
#if defined(CONFIG_R8A7796)
		if (!ret) {
			/* select memory type */
			bank_num = 0;
			for (i = 0; i < 4; i++) {
				if (dram_conf_addr.address[i])
					bank_num++;
				else
					continue;
			}
			bank_size = dram_conf_addr.size[0];
			if (!strcmp(dt_fit.target_name, "r8a7796-salvator-xs-u-boot") &&
			    bank_num == 2 && bank_size == 0x80000000) {
				return 0;
			} else if (!strcmp(dt_fit.target_name,
					   "r8a7796-salvator-xs-2x4g-u-boot") &&
				   bank_num == 2 && bank_size == 0x100000000) {
				return 0;
			}
		}
		/* else works default : return -1 */
#endif
#if defined(CONFIG_R8A77965)
		if (!strcmp(dt_fit.target_name, "r8a77965-salvator-xs-u-boot"))
			return 0;
#endif
		break;
	}

	return -1;
}
#endif
