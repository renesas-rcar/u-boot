/*
 * board/renesas/salvator-x/salvator-x.c
 *     This file is Salvator-X/Salvator-XS board support.
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
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

#define SCIF2_MSTP310	(1 << 10)
#define ETHERAVB_MSTP812	(1 << 12)
#define DVFS_MSTP926	(1 << 26)
#define SD0_MSTP314	(1 << 14)
#define SD1_MSTP313	(1 << 13)
#define SD2_MSTP312	(1 << 12)	/* either MMC0 */
#define SD3_MSTP311	(1 << 11)	/* either MMC1 */
#define GPIO2_MSTP910	(1 << 10)	/* GPIO2: includes assignment of AVB */
#define GPIO3_MSTP909	(1 << 9)	/* GPIO3: includes assignment of SDHI3 */
#define GPIO5_MSTP907	(1 << 7)	/* GPIO5: includes assignment of SDHI0/1/2 */
#define GPIO6_MSTP906	(1 << 6)	/* GPIO6: includes assignment of USB */

#define SD0CKCR		0xE6150074
#define SD1CKCR		0xE6150078
#define SD2CKCR		0xE6150268
#define SD3CKCR		0xE615026C

int board_early_init_f(void)
{
	int freq;

	rcar_prr_init();

	/* SCIF2 */
	mstp_clrbits_le32(SMSTPCR3, SMSTPCR3, SCIF2_MSTP310);
	mstp_wait_clrbits_le32(MSTPSR3, SCIF2_MSTP310);
	/* EHTERAVB */
	mstp_clrbits_le32(SMSTPCR8, SMSTPCR8, ETHERAVB_MSTP812);
	mstp_wait_clrbits_le32(MSTPSR8, ETHERAVB_MSTP812);
	/* eMMC */
	mstp_clrbits_le32(SMSTPCR3, SMSTPCR3, SD1_MSTP313 | SD2_MSTP312);
	mstp_wait_clrbits_le32(MSTPSR3, SD1_MSTP313 | SD2_MSTP312);
	/* SDHI0, 3 */
	mstp_clrbits_le32(SMSTPCR3, SMSTPCR3, SD0_MSTP314 | SD3_MSTP311);
	mstp_wait_clrbits_le32(MSTPSR3, SD0_MSTP314 | SD3_MSTP311);
#if defined(CONFIG_R8A7795)
	if (rcar_is_legacy()) {
		mstp_clrbits_le32(SMSTPCR9, SMSTPCR9,
				  GPIO2_MSTP910 | GPIO3_MSTP909 |
				  GPIO5_MSTP907);
		mstp_wait_clrbits_le32(MSTPSR9,
				       GPIO2_MSTP910 | GPIO3_MSTP909 |
				       GPIO5_MSTP907);
	} else {
		mstp_clrbits_le32(SMSTPCR9, SMSTPCR9,
				  GPIO2_MSTP910 | GPIO3_MSTP909 |
				  GPIO5_MSTP907 | GPIO6_MSTP906);
		mstp_wait_clrbits_le32(MSTPSR9,
				       GPIO2_MSTP910 | GPIO3_MSTP909 |
				       GPIO5_MSTP907 | GPIO6_MSTP906);
	}
#elif defined(CONFIG_R8A7796X)
	mstp_clrbits_le32(SMSTPCR9, SMSTPCR9,
			  GPIO2_MSTP910 | GPIO3_MSTP909 |
			  GPIO5_MSTP907 | GPIO6_MSTP906);
	mstp_wait_clrbits_le32(MSTPSR9,
			       GPIO2_MSTP910 | GPIO3_MSTP909 |
			       GPIO5_MSTP907 | GPIO6_MSTP906);
#endif
	freq = rcar_get_sdhi_config_clk();
	writel(freq, SD0CKCR);
	writel(freq, SD1CKCR);
	writel(freq, SD2CKCR);
	writel(freq, SD3CKCR);

#if defined(CONFIG_SYS_I2C) && defined(CONFIG_SYS_I2C_SH)
	/* DVFS for reset */
	mstp_clrbits_le32(SMSTPCR9, SMSTPCR9, DVFS_MSTP926);
	mstp_wait_clrbits_le32(MSTPSR9, DVFS_MSTP926);
#endif
	return 0;
}

void board_cleanup_preboot_os(void)
{
	/* Stop the used sys-module clocks */
	/* EHTERAVB */
	mstp_setbits_le32(SMSTPCR8, SMSTPCR8, ETHERAVB_MSTP812);
	/* eMMC */
	mstp_setbits_le32(SMSTPCR3, SMSTPCR3, SD1_MSTP313 | SD2_MSTP312);
	/* SDHI0, 3 */
	mstp_setbits_le32(SMSTPCR3, SMSTPCR3, SD0_MSTP314 | SD3_MSTP311);
#if defined(CONFIG_SYS_I2C) && defined(CONFIG_SYS_I2C_SH)
	/* DVFS for reset */
	mstp_setbits_le32(SMSTPCR9, SMSTPCR9, DVFS_MSTP926);
#endif
#if defined(CONFIG_R8A7795)
	if (rcar_is_legacy()) {
		mstp_setbits_le32(SMSTPCR9, SMSTPCR9,
				  GPIO2_MSTP910 | GPIO3_MSTP909 |
				  GPIO5_MSTP907);
	} else {
		mstp_setbits_le32(SMSTPCR9, SMSTPCR9,
				  GPIO2_MSTP910 | GPIO3_MSTP909 |
				  GPIO5_MSTP907 | GPIO6_MSTP906);
	}
#elif defined(CONFIG_R8A7796X)
	mstp_setbits_le32(SMSTPCR9, SMSTPCR9,
			  GPIO2_MSTP910 | GPIO3_MSTP909 |
			  GPIO5_MSTP907 | GPIO6_MSTP906);
#endif
	/* Supply SCIF2 (don't stop SCIF2_MSTP310) */
}

int board_init(void)
{
	u32 val;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	/* Init PFC controller */
	pinmux_init();

	/* USB1 pull-up */
	val = readl(PFC_PUEN6) | PUEN_USB1_OVC | PUEN_USB1_PWEN;
	writel(val, PFC_PUEN6);

#ifdef CONFIG_RAVB
	/* EtherAVB Enable */
#if defined(CONFIG_R8A7795)
	if (rcar_is_legacy()) {
		/* GPSR2 */
		gpio_request(ES_GPIO_GFN_AVB_AVTP_CAPTURE_A, NULL);
		gpio_request(ES_GPIO_GFN_AVB_AVTP_MATCH_A, NULL);
		gpio_request(ES_GPIO_GFN_AVB_LINK, NULL);
		gpio_request(ES_GPIO_GFN_AVB_PHY_INT, NULL);
		gpio_request(ES_GPIO_GFN_AVB_MAGIC, NULL);
		gpio_request(ES_GPIO_GFN_AVB_MDC, NULL);

		/* IPSR0 */
		gpio_request(ES_GPIO_IFN_AVB_MDC, NULL);
		gpio_request(ES_GPIO_IFN_AVB_MAGIC, NULL);
		gpio_request(ES_GPIO_IFN_AVB_PHY_INT, NULL);
		gpio_request(ES_GPIO_IFN_AVB_LINK, NULL);
		gpio_request(ES_GPIO_IFN_AVB_AVTP_MATCH_A, NULL);
		gpio_request(ES_GPIO_IFN_AVB_AVTP_CAPTURE_A, NULL);
		/* IPSR1 */
		gpio_request(ES_GPIO_FN_AVB_AVTP_PPS, NULL);
		/* IPSR2 */
		gpio_request(ES_GPIO_FN_AVB_AVTP_MATCH_B, NULL);
		/* IPSR3 */
		gpio_request(ES_GPIO_FN_AVB_AVTP_CAPTURE_B, NULL);

		/* AVB_PHY_RST */
		gpio_request(ES_GPIO_GP_2_10, NULL);
		gpio_direction_output(ES_GPIO_GP_2_10, 0);
		mdelay(20);
		gpio_set_value(ES_GPIO_GP_2_10, 1);
		udelay(1);
	} else {
		/* GPSR2 */
		gpio_request(GPIO_GFN_AVB_AVTP_CAPTURE_A, NULL);
		gpio_request(GPIO_GFN_AVB_AVTP_MATCH_A, NULL);
		gpio_request(GPIO_GFN_AVB_LINK, NULL);
		gpio_request(GPIO_GFN_AVB_PHY_INT, NULL);
		gpio_request(GPIO_GFN_AVB_MAGIC, NULL);
		gpio_request(GPIO_GFN_AVB_MDC, NULL);

		/* IPSR0 */
		gpio_request(GPIO_IFN_AVB_MDC, NULL);
		gpio_request(GPIO_IFN_AVB_MAGIC, NULL);
		gpio_request(GPIO_IFN_AVB_PHY_INT, NULL);
		gpio_request(GPIO_IFN_AVB_LINK, NULL);
		gpio_request(GPIO_IFN_AVB_AVTP_MATCH_A, NULL);
		gpio_request(GPIO_IFN_AVB_AVTP_CAPTURE_A, NULL);
		/* IPSR1 */
		gpio_request(GPIO_FN_AVB_AVTP_PPS, NULL);
		/* IPSR2 */
		gpio_request(GPIO_FN_AVB_AVTP_MATCH_B, NULL);
		/* IPSR3 */
		gpio_request(GPIO_FN_AVB_AVTP_CAPTURE_B, NULL);

		/* USB2_OVC */
		gpio_request(GPIO_GP_6_15, NULL);
		gpio_direction_input(GPIO_GP_6_15);

		/* USB2_PWEN */
		gpio_request(GPIO_GP_6_14, NULL);
		gpio_direction_output(GPIO_GP_6_14, 1);
		gpio_set_value(GPIO_GP_6_14, 1);

		/* AVB_PHY_RST */
		gpio_request(GPIO_GP_2_10, NULL);
		gpio_direction_output(GPIO_GP_2_10, 0);
		mdelay(20);
		gpio_set_value(GPIO_GP_2_10, 1);
		udelay(1);
	}
#elif defined(CONFIG_R8A7796X)
	/* GPSR2 */
	gpio_request(GPIO_GFN_AVB_AVTP_CAPTURE_A, NULL);
	gpio_request(GPIO_GFN_AVB_AVTP_MATCH_A, NULL);
	gpio_request(GPIO_GFN_AVB_LINK, NULL);
	gpio_request(GPIO_GFN_AVB_PHY_INT, NULL);
	gpio_request(GPIO_GFN_AVB_MAGIC, NULL);
	gpio_request(GPIO_GFN_AVB_MDC, NULL);

	/* IPSR0 */
	gpio_request(GPIO_IFN_AVB_MDC, NULL);
	gpio_request(GPIO_IFN_AVB_MAGIC, NULL);
	gpio_request(GPIO_IFN_AVB_PHY_INT, NULL);
	gpio_request(GPIO_IFN_AVB_LINK, NULL);
	gpio_request(GPIO_IFN_AVB_AVTP_MATCH_A, NULL);
	gpio_request(GPIO_IFN_AVB_AVTP_CAPTURE_A, NULL);
	/* IPSR1 */
	gpio_request(GPIO_FN_AVB_AVTP_PPS, NULL);
	/* IPSR2 */
	gpio_request(GPIO_FN_AVB_AVTP_MATCH_B, NULL);
	/* IPSR3 */
	gpio_request(GPIO_FN_AVB_AVTP_CAPTURE_B, NULL);

	/* AVB_PHY_RST */
	gpio_request(GPIO_GP_2_10, NULL);
	gpio_direction_output(GPIO_GP_2_10, 0);
	mdelay(20);
	gpio_set_value(GPIO_GP_2_10, 1);
	udelay(1);
#endif

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

/* Salvator-X has KSZ9031RNX */
/* Tri-color dual-LED mode(Pin 41 pull-down) */
int board_phy_config(struct phy_device *phydev)
{
	/* hardware use default(Tri-color:0) setting. */

	return 0;
}

int board_mmc_init(bd_t *bis)
{
	int ret = -ENODEV;

#ifdef CONFIG_SH_SDHI

#if defined(CONFIG_R8A7795)
	if (rcar_is_legacy()) {
		/* SDHI0 */
		gpio_request(ES_GPIO_GFN_SD0_DAT0, NULL);
		gpio_request(ES_GPIO_GFN_SD0_DAT1, NULL);
		gpio_request(ES_GPIO_GFN_SD0_DAT2, NULL);
		gpio_request(ES_GPIO_GFN_SD0_DAT3, NULL);
		gpio_request(ES_GPIO_GFN_SD0_CLK, NULL);
		gpio_request(ES_GPIO_GFN_SD0_CMD, NULL);
		gpio_request(ES_GPIO_GFN_SD0_CD, NULL);
		gpio_request(ES_GPIO_GFN_SD0_WP, NULL);

		gpio_request(ES_GPIO_GP_5_2, NULL);
		gpio_request(ES_GPIO_GP_5_1, NULL);
		/* power on */
		gpio_direction_output(ES_GPIO_GP_5_2, 1);
		/* 1: 3.3V, 0: 1.8V */
		gpio_direction_output(ES_GPIO_GP_5_1, 1);

		ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI0_BASE, 0,
				   SH_SDHI_QUIRK_64BIT_BUF);
		if (ret)
			return ret;

		/* SDHI1/SDHI2 eMMC */
		gpio_request(ES_GPIO_GFN_SD1_DAT0, NULL);
		gpio_request(ES_GPIO_GFN_SD1_DAT1, NULL);
		gpio_request(ES_GPIO_GFN_SD1_DAT2, NULL);
		gpio_request(ES_GPIO_GFN_SD1_DAT3, NULL);
		gpio_request(ES_GPIO_GFN_SD2_DAT0, NULL);
		gpio_request(ES_GPIO_GFN_SD2_DAT1, NULL);
		gpio_request(ES_GPIO_GFN_SD2_DAT2, NULL);
		gpio_request(ES_GPIO_GFN_SD2_DAT3, NULL);
		gpio_request(ES_GPIO_GFN_SD2_CLK, NULL);
		gpio_request(ES_GPIO_FN_SD2_CMD, NULL);

		gpio_request(ES_GPIO_GP_5_3, NULL);
		gpio_request(ES_GPIO_GP_5_9, NULL);
		/* 1: 3.3V, 0: 1.8V */
		gpio_direction_output(ES_GPIO_GP_5_3, 0);
		gpio_direction_output(ES_GPIO_GP_5_9, 0);

		ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI2_BASE, 1,
				   SH_SDHI_QUIRK_64BIT_BUF);
		if (ret)
			return ret;

		/* SDHI3 */
		gpio_request(ES_GPIO_FN_SD3_DAT0, NULL);	/* GP_4_9 */
		gpio_request(ES_GPIO_FN_SD3_DAT1, NULL);	/* GP_4_10 */
		gpio_request(ES_GPIO_FN_SD3_DAT2, NULL);	/* GP_4_11 */
		gpio_request(ES_GPIO_FN_SD3_DAT3, NULL);	/* GP_4_12 */
		gpio_request(ES_GPIO_FN_SD3_CLK, NULL);	/* GP_4_7 */
		gpio_request(ES_GPIO_FN_SD3_CMD, NULL);	/* GP_4_8 */
		gpio_request(ES_GPIO_FN_SD3_CD, NULL);	/* GP_4_15 */
		gpio_request(ES_GPIO_FN_SD3_WP, NULL);	/* GP_4_16 */

		gpio_request(ES_GPIO_GP_3_15, NULL);
		gpio_request(ES_GPIO_GP_3_14, NULL);
		/* power on */
		gpio_direction_output(ES_GPIO_GP_3_15, 1);
		/* 1: 3.3V, 0: 1.8V */
		gpio_direction_output(ES_GPIO_GP_3_14, 1);

		ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI3_BASE, 2,
				   SH_SDHI_QUIRK_64BIT_BUF);
	} else {
		/* SDHI0 */
		gpio_request(GPIO_GFN_SD0_DAT0, NULL);
		gpio_request(GPIO_GFN_SD0_DAT1, NULL);
		gpio_request(GPIO_GFN_SD0_DAT2, NULL);
		gpio_request(GPIO_GFN_SD0_DAT3, NULL);
		gpio_request(GPIO_GFN_SD0_CLK, NULL);
		gpio_request(GPIO_GFN_SD0_CMD, NULL);
		gpio_request(GPIO_GFN_SD0_CD, NULL);
		gpio_request(GPIO_GFN_SD0_WP, NULL);

		gpio_request(GPIO_GP_5_2, NULL);
		gpio_request(GPIO_GP_5_1, NULL);
		gpio_direction_output(GPIO_GP_5_2, 1);	/* power on */
		gpio_direction_output(GPIO_GP_5_1, 1);	/* 1: 3.3V, 0: 1.8V */

		ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI0_BASE, 0,
				   SH_SDHI_QUIRK_64BIT_BUF);
		if (ret)
			return ret;

		/* SDHI1/SDHI2 eMMC */
		gpio_request(GPIO_GFN_SD1_DAT0, NULL);
		gpio_request(GPIO_GFN_SD1_DAT1, NULL);
		gpio_request(GPIO_GFN_SD1_DAT2, NULL);
		gpio_request(GPIO_GFN_SD1_DAT3, NULL);
		gpio_request(GPIO_GFN_SD2_DAT0, NULL);
		gpio_request(GPIO_GFN_SD2_DAT1, NULL);
		gpio_request(GPIO_GFN_SD2_DAT2, NULL);
		gpio_request(GPIO_GFN_SD2_DAT3, NULL);
		gpio_request(GPIO_GFN_SD2_CLK, NULL);
		gpio_request(GPIO_GFN_SD2_CMD, NULL);

		gpio_request(GPIO_GP_5_3, NULL);
		gpio_request(GPIO_GP_5_9, NULL);
		gpio_direction_output(GPIO_GP_5_3, 0);	/* 1: 3.3V, 0: 1.8V */
		gpio_direction_output(GPIO_GP_5_9, 0);	/* 1: 3.3V, 0: 1.8V */

		ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI2_BASE, 1,
				   SH_SDHI_QUIRK_64BIT_BUF);
		if (ret)
			return ret;

		/* SDHI3 */
		gpio_request(GPIO_GFN_SD3_DAT0, NULL);	/* GP_4_9 */
		gpio_request(GPIO_GFN_SD3_DAT1, NULL);	/* GP_4_10 */
		gpio_request(GPIO_GFN_SD3_DAT2, NULL);	/* GP_4_11 */
		gpio_request(GPIO_GFN_SD3_DAT3, NULL);	/* GP_4_12 */
		gpio_request(GPIO_GFN_SD3_CLK, NULL);	/* GP_4_7 */
		gpio_request(GPIO_GFN_SD3_CMD, NULL);	/* GP_4_8 */
		/* IPSR10 */
		gpio_request(GPIO_FN_SD3_CD, NULL);
		gpio_request(GPIO_FN_SD3_WP, NULL);

		gpio_request(GPIO_GP_3_15, NULL);
		gpio_request(GPIO_GP_3_14, NULL);
		gpio_direction_output(GPIO_GP_3_15, 1);	/* power on */
		gpio_direction_output(GPIO_GP_3_14, 1);	/* 1: 3.3V, 0: 1.8V */

		ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI3_BASE, 2,
				   SH_SDHI_QUIRK_64BIT_BUF);
	}
#elif defined(CONFIG_R8A7796X)
	/* SDHI0 */
	gpio_request(GPIO_GFN_SD0_DAT0, NULL);
	gpio_request(GPIO_GFN_SD0_DAT1, NULL);
	gpio_request(GPIO_GFN_SD0_DAT2, NULL);
	gpio_request(GPIO_GFN_SD0_DAT3, NULL);
	gpio_request(GPIO_GFN_SD0_CLK, NULL);
	gpio_request(GPIO_GFN_SD0_CMD, NULL);
	gpio_request(GPIO_GFN_SD0_CD, NULL);
	gpio_request(GPIO_GFN_SD0_WP, NULL);

	gpio_request(GPIO_GP_5_2, NULL);
	gpio_request(GPIO_GP_5_1, NULL);
	gpio_direction_output(GPIO_GP_5_2, 1);	/* power on */
	gpio_direction_output(GPIO_GP_5_1, 1);	/* 1: 3.3V, 0: 1.8V */

	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI0_BASE, 0,
			   SH_SDHI_QUIRK_64BIT_BUF);
	if (ret)
		return ret;

	/* SDHI1/SDHI2 eMMC */
	gpio_request(GPIO_GFN_SD1_DAT0, NULL);
	gpio_request(GPIO_GFN_SD1_DAT1, NULL);
	gpio_request(GPIO_GFN_SD1_DAT2, NULL);
	gpio_request(GPIO_GFN_SD1_DAT3, NULL);
	gpio_request(GPIO_GFN_SD2_DAT0, NULL);
	gpio_request(GPIO_GFN_SD2_DAT1, NULL);
	gpio_request(GPIO_GFN_SD2_DAT2, NULL);
	gpio_request(GPIO_GFN_SD2_DAT3, NULL);
	gpio_request(GPIO_GFN_SD2_CLK, NULL);
	gpio_request(GPIO_FN_SD2_CMD, NULL);

	gpio_request(GPIO_GP_5_3, NULL);
	gpio_request(GPIO_GP_5_9, NULL);
	gpio_direction_output(GPIO_GP_5_3, 0);	/* 1: 3.3V, 0: 1.8V */
	gpio_direction_output(GPIO_GP_5_9, 0);	/* 1: 3.3V, 0: 1.8V */

	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI2_BASE, 1,
			   SH_SDHI_QUIRK_64BIT_BUF);
	if (ret)
		return ret;

	/* SDHI3 */
	gpio_request(GPIO_FN_SD3_DAT0, NULL);	/* GP_4_9 */
	gpio_request(GPIO_FN_SD3_DAT1, NULL);	/* GP_4_10 */
	gpio_request(GPIO_FN_SD3_DAT2, NULL);	/* GP_4_11 */
	gpio_request(GPIO_FN_SD3_DAT3, NULL);	/* GP_4_12 */
	gpio_request(GPIO_FN_SD3_CLK, NULL);	/* GP_4_7 */
	gpio_request(GPIO_FN_SD3_CMD, NULL);	/* GP_4_8 */
	gpio_request(GPIO_FN_SD3_CD, NULL);	/* GP_4_15 */
	gpio_request(GPIO_FN_SD3_WP, NULL);	/* GP_4_16 */

	gpio_request(GPIO_GP_3_15, NULL);
	gpio_request(GPIO_GP_3_14, NULL);
	gpio_direction_output(GPIO_GP_3_15, 1);	/* power on */
	gpio_direction_output(GPIO_GP_3_14, 1);	/* 1: 3.3V, 0: 1.8V */

	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI3_BASE, 2,
			   SH_SDHI_QUIRK_64BIT_BUF);
#endif

#endif
	return ret;
}


int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
#if (CONFIG_NR_DRAM_BANKS >= 2)
	gd->ram_size += PHYS_SDRAM_2_SIZE;
#endif
#if (CONFIG_NR_DRAM_BANKS >= 3)
	gd->ram_size += PHYS_SDRAM_3_SIZE;
#endif
#if (CONFIG_NR_DRAM_BANKS >= 4)
	gd->ram_size += PHYS_SDRAM_4_SIZE;
#endif

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
#if (CONFIG_NR_DRAM_BANKS >= 2)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
#endif
#if (CONFIG_NR_DRAM_BANKS >= 3)
	gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
	gd->bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
#endif
#if (CONFIG_NR_DRAM_BANKS >= 4)
	gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
	gd->bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;
#endif
}

const struct rcar_sysinfo sysinfo = {
	CONFIG_RCAR_BOARD_STRING
};

void reset_cpu(ulong addr)
{
#if defined(CONFIG_SYS_I2C) && defined(CONFIG_SYS_I2C_SH)
	i2c_reg_write(CONFIG_SYS_I2C_POWERIC_ADDR, 0x20, 0x80);
#endif
}

#if defined(CONFIG_DISPLAY_BOARDINFO)
int checkboard(void)
{
	printf("Board: %s\n", sysinfo.board_string);
	return 0;
}
#endif
