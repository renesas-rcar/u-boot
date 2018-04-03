/*
 * board/renesas/draak/draak.c
 *     This file is Draak board support.
 *
 * Copyright (C) 2015-2018 Renesas Electronics Corporation
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
#include <asm/arch/sh_sdhi.h>		/* D3 not support */
#include <i2c.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#define SCIF2_MSTP310		(1 << 10)
#define ETHERAVB_MSTP812	(1 << 12)
#define SD2_MSTP312			(1 << 12)	/* either MMC0 */

#define SD2CKCR				0xE6150268	/* eMMC CLK */

int board_early_init_f(void)
{
	int freq;

	rcar_prr_init();

	/* SCIF2 */
	mstp_clrbits_le32(SMSTPCR3, SMSTPCR3, SCIF2_MSTP310);
	/* EHTERAVB */
	mstp_clrbits_le32(SMSTPCR8, SMSTPCR8, ETHERAVB_MSTP812);
	/* eMMC */
	mstp_clrbits_le32(SMSTPCR3, SMSTPCR3, SD2_MSTP312);

	freq = rcar_get_sdhi_config_clk();
	writel(freq, SD2CKCR);		/* eMMC0 */

	return 0;
}


DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	u32 val;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	/* Init PFC controller */
	pinmux_init();

	/* USB0 pull-up */
	val = readl(PFC_PUEN0) | PUEN_USB0_OVC | PUEN_USB0_PWEN;
	writel(val, PFC_PUEN0);

#ifdef CONFIG_RAVB
	/* EtherAVB Enable */
	/* GPSR2 */
	gpio_request(GPIO_FN_AVB0_LINK, NULL);
	gpio_request(GPIO_FN_AVB0_PHY_INT, NULL);
	gpio_request(GPIO_FN_AVB0_MAGIC, NULL);
	gpio_request(GPIO_FN_AVB0_MDC, NULL);
	gpio_request(GPIO_FN_AVB0_MDIO, NULL);
	gpio_request(GPIO_FN_AVB0_TXCREFCLK, NULL);
	gpio_request(GPIO_FN_AVB0_TD3, NULL);
	gpio_request(GPIO_FN_AVB0_TD2, NULL);
	gpio_request(GPIO_FN_AVB0_TD1, NULL);
	gpio_request(GPIO_FN_AVB0_TD0, NULL);
	gpio_request(GPIO_FN_AVB0_TXC, NULL);
	gpio_request(GPIO_FN_AVB0_TX_CTL, NULL);
	gpio_request(GPIO_FN_AVB0_RD3, NULL);
	gpio_request(GPIO_FN_AVB0_RD2, NULL);
	gpio_request(GPIO_FN_AVB0_RD1, NULL);
	gpio_request(GPIO_FN_AVB0_RD0, NULL);
	gpio_request(GPIO_FN_AVB0_RXC, NULL);
	gpio_request(GPIO_FN_AVB0_RX_CTL, NULL);

	/* IPSR11 */
	gpio_request(GPIO_FN_AVB0_AVTP_MATCH_B, NULL);
	gpio_request(GPIO_FN_AVB0_AVTP_CAPTURE_B, NULL);
	gpio_request(GPIO_FN_AVB0_AVTP_PPS_B, NULL);

	/* AVB_PHY_RST */
	gpio_request(GPIO_GP_5_18, NULL);
	gpio_direction_output(GPIO_GP_5_18, 0);
	mdelay(20);
	gpio_set_value(GPIO_GP_5_18, 1);
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

/* Draak has KSZ9031RNX */
/* Tri-color dual-LED mode(Pin 41 pull-down) */
int board_phy_config(struct phy_device *phydev)
{
	/* hardware use default(Tri-color:0) setting. */

	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int ret = -ENODEV;

	/* MMC */
	/* IPSR8 */
	gpio_request(GPIO_FN_MMC_CLK, NULL);
	gpio_request(GPIO_FN_MMC_CMD, NULL);

	/* IPSR9 */
	gpio_request(GPIO_FN_MMC_D0, NULL);
	gpio_request(GPIO_FN_MMC_D1, NULL);
	gpio_request(GPIO_FN_MMC_D2, NULL);
	gpio_request(GPIO_FN_MMC_D3, NULL);
	gpio_request(GPIO_FN_MMC_D4, NULL);
	gpio_request(GPIO_FN_MMC_D5, NULL);
	gpio_request(GPIO_FN_MMC_D6, NULL);
	gpio_request(GPIO_FN_MMC_D7, NULL);

	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI2_BASE,
			   0, SH_SDHI_QUIRK_64BIT_BUF);

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
