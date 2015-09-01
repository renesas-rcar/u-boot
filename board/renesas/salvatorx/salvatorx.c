/*
 * board/renesas/salvatorx/salvatorx.c
 *     This file is Salvator-X board support.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
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
#include <asm/arch/gpio.h>
#include <asm/arch/rcar_gen3.h>
#include <asm/arch/rcar-mstp.h>
#include <i2c.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

#define CPGWPCR	0xE6150904
#define CPGWPR  0xE615090C

#define CLK2MHZ(clk)	(clk / 1000 / 1000)
void s_init(void)
{
	struct rcar_rwdt *rwdt = (struct rcar_rwdt *)RWDT_BASE;
	struct rcar_swdt *swdt = (struct rcar_swdt *)SWDT_BASE;

	/* Watchdog init */
	writel(0xA5A5A500, &rwdt->rwtcsra);
	writel(0xA5A5A500, &swdt->swtcsra);

	writel(0xA5A50000, CPGWPCR);
	writel(0xFFFFFFFF, CPGWPR);
}

#define TMU0_MSTP125	(1 << 25)		/* secure */
#define TMU1_MSTP124	(1 << 24)		/* non-secure */
#define SCIF2_MSTP310	(1 << 10)
#define ETHERAVB_MSTP812	(1 << 12)
#define SD0_MSTP314	(1 << 14)
#define SD1_MSTP313	(1 << 13)
#define SD2_MSTP312	(1 << 12)		/* either MMC0 */
#define SD3_MSTP311	(1 << 11)		/* either MMC1 */

#define SD3CKCR		0xE615026C
#define SD_SRCFC_DIV1_DIV4	(0 << 2)
#define SD_SRCFC_DIV2_DIV8	(1 << 2)
#define SD_SRCFC_DIV4_DIV16	(2 << 2)
#define SD_SRCFC_STOP_DIV32	(3 << 2)
#define SD_SRCFC_STOP_DIV64	(4 << 2)
#define SD_FC_DIV2		0
#define SD_FC_DIV4		1
#define SD_FC_DIV5		2
#define SDH800_SD200		(SD_SRCFC_DIV1_DIV4 | SD_FC_DIV4)
#define SDH400_SD200		(SD_SRCFC_DIV2_DIV8 | SD_FC_DIV2)

int board_early_init_f(void)
{
	/* TMU0,1 */		/* which use ? */
	mstp_clrbits_le32(MSTPSR1, SMSTPCR1, TMU0_MSTP125 | TMU1_MSTP124);
	/* SCIF2 */
	mstp_clrbits_le32(MSTPSR3, SMSTPCR3, SCIF2_MSTP310);
	/* EHTERAVB */
	mstp_clrbits_le32(MSTPSR8, SMSTPCR8, ETHERAVB_MSTP812);
	/* eMMC */
	mstp_clrbits_le32(MSTPSR3, SMSTPCR3, SD1_MSTP313 | SD2_MSTP312);
	/* SDHI0, 3 */
	mstp_clrbits_le32(MSTPSR3, SMSTPCR3, SD0_MSTP314 | SD3_MSTP311);

	writel(SDH800_SD200, SD3CKCR);

	return 0;
}

/* PFC.h */
#define	PFC_PMMR	0xE6060000	/* R/W 32 LSI Multiplexed Pin Setting Mask Register */
#define	PFC_DRVCTRL2	0xE6060308	/* R/W 32 DRV control register2 */
#define	PFC_DRVCTRL3	0xE606030C	/* R/W 32 DRV control register3 */

DECLARE_GLOBAL_DATA_PTR;
int board_init(void)
{
#ifdef CONFIG_RAVB
	u32 val;
#endif
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	/* Init PFC controller */
	r8a7795_pinmux_init();

#ifdef CONFIG_RAVB
	/* EtherAVB Enable */

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

	val = readl(PFC_DRVCTRL2);
	val &= ~0x00000777;
	val |=  0x00000333;
	writel(~val, PFC_PMMR);
	writel(val, PFC_DRVCTRL2);

	val = readl(PFC_DRVCTRL3);
	val &= ~0x77700000;
	val |=  0x33300000;
	writel(~val, PFC_PMMR);
	writel(val, PFC_DRVCTRL3);

	/* AVB_PHY_RST */
	gpio_request(GPIO_GP_2_10, NULL);
	gpio_direction_output(GPIO_GP_2_10, 0);
	mdelay(20);
	gpio_set_value(GPIO_GP_2_10, 1);
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

#if 0 /* gen3 */
	/* MMC */
	gpio_request(GPIO_GP_3_11, NULL);
	gpio_request(GPIO_GP_3_10, NULL);
	gpio_request(GPIO_GP_3_9, NULL);
	gpio_request(GPIO_GP_3_8, NULL);

	gpio_request(GPIO_GP_4_5, NULL);
	gpio_request(GPIO_GP_4_4, NULL);
	gpio_request(GPIO_GP_4_3, NULL);
	gpio_request(GPIO_GP_4_2, NULL);
	gpio_request(GPIO_GP_4_1, NULL);
	gpio_request(GPIO_GP_4_0, NULL);
	gpio_request(GPIO_GP_4_6, NULL);

	ret = mmcif_mmc_init();
#endif

#ifdef CONFIG_SH_SDHI
	/* SDHI0 */
	gpio_request(GPIO_GFN_SD0_DAT0, NULL);
	gpio_direction_output(GPIO_GFN_SD0_DAT0, 1);
	gpio_request(GPIO_GFN_SD0_DAT1, NULL);
	gpio_direction_output(GPIO_GFN_SD0_DAT1, 1);
	gpio_request(GPIO_GFN_SD0_DAT2, NULL);
	gpio_direction_output(GPIO_GFN_SD0_DAT2, 1);
	gpio_request(GPIO_GFN_SD0_DAT3, NULL);
	gpio_direction_output(GPIO_GFN_SD0_DAT3, 1);
	gpio_request(GPIO_GFN_SD0_CLK, NULL);
	gpio_direction_output(GPIO_GFN_SD0_CLK, 1);
	gpio_request(GPIO_GFN_SD0_CMD, NULL);
	gpio_direction_output(GPIO_GFN_SD0_CMD, 1);
	gpio_request(GPIO_GFN_SD0_CD, NULL);
	gpio_direction_output(GPIO_GFN_SD0_CD, 1);
	gpio_request(GPIO_GFN_SD0_WP, NULL);
	gpio_direction_output(GPIO_GFN_SD0_WP, 1);

	/* SDHI3 */
	gpio_request(GPIO_FN_SD3_DAT0, NULL);	/* GP_4_9 */
	gpio_direction_output(GPIO_FN_SD3_DAT0, 1);
	gpio_request(GPIO_FN_SD3_DAT1, NULL);	/* GP_4_10 */
	gpio_direction_output(GPIO_FN_SD3_DAT1, 1);
	gpio_request(GPIO_FN_SD3_DAT2, NULL);	/* GP_4_11 */
	gpio_direction_output(GPIO_FN_SD3_DAT2, 1);
	gpio_request(GPIO_FN_SD3_DAT3, NULL);	/* GP_4_12 */
	gpio_direction_output(GPIO_FN_SD3_DAT3, 1);
	gpio_request(GPIO_FN_SD3_CLK, NULL);	/* GP_4_7 */
	gpio_direction_output(GPIO_FN_SD3_CLK, 1);
	gpio_request(GPIO_FN_SD3_CMD, NULL);	/* GP_4_8 */
	gpio_direction_output(GPIO_FN_SD3_CMD, 1);
	gpio_request(GPIO_FN_SD3_CD, NULL);	/* GP_4_15 */
	gpio_direction_output(GPIO_FN_SD3_CD, 1);
	gpio_request(GPIO_FN_SD3_WP, NULL);	/* GP_4_16 */
	gpio_direction_output(GPIO_FN_SD3_WP, 1);

	/* SDHI0 */
	gpio_request(GPIO_GP_5_2, NULL);
	gpio_request(GPIO_GP_5_1, NULL);
	gpio_direction_output(GPIO_GP_5_2, 1);	/* power on */
	gpio_direction_output(GPIO_GP_5_1, 1);	/* 1: 3.3V, 0: 1.8V */

	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI0_BASE, 0,
			   SH_SDHI_QUIRK_16BIT_BUF);
	if (ret)
		return ret;

	/* SDHI 3 */
	gpio_request(GPIO_GP_3_15, NULL);
	gpio_request(GPIO_GP_3_14, NULL);
	gpio_direction_output(GPIO_GP_3_15, 1);	/* power on */
	gpio_direction_output(GPIO_GP_3_14, 1);	/* 1: 3.3V, 0: 1.8V */

	ret = sh_sdhi_init(CONFIG_SYS_SH_SDHI3_BASE, 2, 0);
#endif
	return ret;
}


int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	return 0;
}

const struct rcar_sysinfo sysinfo = {
	CONFIG_RCAR_BOARD_STRING
};


#define RST_BASE	0xE6160000
#define RST_CA57RESCNT	(RST_BASE + 0x40)
#define RST_CA53RESCNT	(RST_BASE + 0x44)
#define RST_RSTOUTCR	(RST_BASE + 0x58)


void reset_cpu(ulong addr)
{
	/* only CA57 ? */
	writel(0xA5A5000F, RST_CA57RESCNT);
}

static const struct sh_serial_platdata serial_platdata = {
	.base = SCIF1_BASE,
	.type = PORT_SCIF,
	.clk = 14745600,		/* 0xE10000 */
	.clk_mode = EXT_CLK,
};

U_BOOT_DEVICE(salvatorx_serial1) = {
	.name = "serial_sh1",
	.platdata = &serial_platdata,
};
