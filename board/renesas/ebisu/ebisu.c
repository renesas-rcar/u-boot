/*
 * board/renesas/ebisu/ebisu.c
 *     This file is Ebisu board support.
 *
 * Copyright (C) 2018 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <netdev.h>
#include <dm.h>
#include <dm/platform_data/serial_sh.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/arch/prr_depend.h>
#include <asm/arch/gpio.h>
#include <asm/arch/rcar_gen3.h>
#include <asm/arch/rcar-mstp.h>
#include <asm/arch/sh_sdhi.h>
#include <i2c.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#define SCIF2_MSTP310		(1 << 10)
#define SD0_MSTP314		(1 << 14)
#define SD1_MSTP313		(1 << 13)
#define SD3_MSTP311		(1 << 11)
#define ETHERAVB_MSTP812	(1 << 12)
#define GPIO1_MSTP911		(1 << 11)
#define GPIO2_MSTP910		(1 << 10)	/* includes assignment of AVB */
#define GPIO6_MSTP906		(1 << 6)	/* includes assignment of USB */

#define SD0CKCR			0xE6150074
#define SD1CKCR			0xE6150078
#define SD3CKCR			0xE615026C	/* eMMC CLK */

int board_early_init_f(void)
{
	int freq;

	rcar_prr_init();

	/* gpio enable for modules */
	mstp_clrbits_le32(SMSTPCR9, SMSTPCR9,
			  GPIO1_MSTP911 | GPIO2_MSTP910 | GPIO6_MSTP906);

	/* SD/eMMC, SCIF */
	mstp_clrbits_le32(SMSTPCR3, SMSTPCR3,
			  SD0_MSTP314 | SD1_MSTP313 | SD3_MSTP311 |
			  SCIF2_MSTP310);
	/* EHTERAVB */
	mstp_clrbits_le32(SMSTPCR8, SMSTPCR8, ETHERAVB_MSTP812);

	freq = rcar_get_sdhi_config_clk();
	writel(freq, SD0CKCR);
	writel(freq, SD1CKCR);
	writel(freq, SD3CKCR);		/* eMMC0 */

	return 0;
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	/* Init PFC controller */
	pinmux_init();

	/* USB0_OVC */
	gpio_request(GPIO_GP_6_12, NULL);
	gpio_direction_input(GPIO_GP_6_12);

	/* UBS0_PWEN */
	gpio_request(GPIO_GP_6_11, NULL);
	gpio_direction_output(GPIO_GP_6_11, 1);
	gpio_set_value(GPIO_GP_6_11, 1);

#ifdef CONFIG_RAVB
	/* EtherAVB Enable */
	/* GPSR2 */
	gpio_request(GPIO_GFN_RDx, NULL);		/* 23 */
	gpio_request(GPIO_FN_AVB_PHY_INT, NULL);	/* 21 */
	gpio_request(GPIO_GFN_AVB_TXCREFCLK, NULL);	/* 20 */
	gpio_request(GPIO_FN_AVB_RD3, NULL);		/* 19 */
	gpio_request(GPIO_GFN_AVB_RD2, NULL);		/* 18 */
	gpio_request(GPIO_GFN_AVB_RD1, NULL);		/* 17 */
	gpio_request(GPIO_GFN_AVB_RD0, NULL);		/* 16 */
	gpio_request(GPIO_FN_AVB_RXC, NULL);		/* 15 */
	gpio_request(GPIO_FN_AVB_RX_CTL, NULL);		/* 14 */
	/* IP1 */
	gpio_request(GPIO_IFN_AVB_RD0, NULL);
	gpio_request(GPIO_IFN_AVB_RD1, NULL);
	gpio_request(GPIO_IFN_AVB_RD2, NULL);
	/* IP2 */
	gpio_request(GPIO_IFN_AVB_TXCREFCLK, NULL);
	gpio_request(GPIO_FN_AVB_MDIO, NULL);
	gpio_request(GPIO_FN_AVB_MDC, NULL);

	/* AVB_PHY_RST */
	gpio_request(GPIO_GP_1_20, NULL);
	gpio_direction_output(GPIO_GP_1_20, 0);
	mdelay(20);
	gpio_set_value(GPIO_GP_1_20, 1);
	udelay(1);
#endif
	return 0;
}

#define MAHR 0xE68005C0
#define MALR 0xE68005C8
int board_eth_init(bd_t *bis)
{
	int ret = -ENODEV;
	u32 val;
	unsigned char enetaddr[6];

	if (!eth_getenv_enetaddr("ethaddr", enetaddr))
		return ret;

	/* Set Mac address */
	val = enetaddr[0] << 24 | enetaddr[1] << 16 |
	    enetaddr[2] << 8 | enetaddr[3];
	writel(val, MAHR);

	val = enetaddr[4] << 8 | enetaddr[5];
	writel(val, MALR);

#ifdef CONFIG_RAVB
	ret = ravb_initialize(bis);
#endif
	return ret;
}

/* Ebisu has KSZ9031RNX */
/* Tri-color dual-LED mode(Pin 41 pull-down) */
int board_phy_config(struct phy_device *phydev)
{
	/* hardware use default(Tri-color:0) setting. */

	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int ret = -ENODEV;

	/* eMMC */
	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI3_BASE, 0,
			   SH_SDHI_QUIRK_64BIT_BUF);
	if (ret)
		return ret;

	/* SD */
	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI0_BASE, 1,
			   SH_SDHI_QUIRK_64BIT_BUF);
	if (ret)
		return ret;

	/* Micro SD */
	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI1_BASE, 2,
			   SH_SDHI_QUIRK_64BIT_BUF);
	if (ret)
		return ret;

	return ret;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

const struct rcar_sysinfo sysinfo = {
	CONFIG_RCAR_BOARD_STRING
};

void reset_cpu(ulong addr)
{
}

#if defined(CONFIG_DISPLAY_BOARDINFO)
int checkboard(void)
{
	printf("Board: %s\n", sysinfo.board_string);
	return 0;
}
#endif
