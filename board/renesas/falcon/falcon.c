// SPDX-License-Identifier: GPL-2.0+
/*
 * board/renesas/falcon/falcon.c
 *     This file is Falcon board support.
 *
 * Copyright (C) 2020 Renesas Electronics Corp.
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/rmobile.h>

DECLARE_GLOBAL_DATA_PTR;

#define CPGWPR  0xE6150000
#define CPGWPCR 0xE6150004

#define	EXTAL_CLK	16666600u
#define CNTCR_BASE	0xE6080000
#define	CNTFID0		(CNTCR_BASE + 0x020)
#define CNTCR_EN	BIT(0)

void init_generic_timer(void)
{
	u32 freq, val;

	/* Set frequency data in CNTFID0 */
	freq = EXTAL_CLK;

	/* Update memory mapped and register based freqency */
	__asm__ volatile ("msr cntfrq_el0, %0" :: "r" (freq));
	writel(freq, CNTFID0);

	/* Enable counter */
	val = readl(CNTCR_BASE);
	writel(val | CNTCR_EN, CNTCR_BASE);
}

/* Distributor Registers */
#define GICD_BASE	0xF1000000

/* ReDistributor Registers for Control and Physical LPIs */
#define GICR_LPI_BASE	0xF1060000
#define GICR_WAKER	0x0014
#define GICR_PWRR	0x0024
#define GICR_LPI_WAKER	(GICR_LPI_BASE + GICR_WAKER)
#define GICR_LPI_PWRR	(GICR_LPI_BASE + GICR_PWRR)

/* ReDistributor Registers for SGIs and PPIs */
#define GICR_SGI_BASE	0xF1070000
#define GICR_IGROUPR0	0x0080

void init_gic_v3(void)
{
	 /* GIC v3 power on */
	writel(0x00000002, (GICR_LPI_PWRR));

	/* Wait till the WAKER_CA_BIT changes to 0 */
	writel(readl(GICR_LPI_WAKER) & ~0x00000002, (GICR_LPI_WAKER));
	while (readl(GICR_LPI_WAKER) & 0x00000004)
		;

	writel(0xffffffff, GICR_SGI_BASE + GICR_IGROUPR0);
}

void s_init(void)
{
	init_generic_timer();
}

#define	PFC_PMMR			0xE6060000
#define	PFC_GP4_BASE			0xE6060000U
#define PFC_GPSR_GP4_DM0		(PFC_GP4_BASE + 0x40U)
#define	PFC_IPSR0_GP4_DM0		(PFC_GP4_BASE + 0x60U)
#define	PFC_IPSR1_GP4_DM0		(PFC_GP4_BASE + 0x64U)
#define	PFC_IPSR2_GP4_DM0		(PFC_GP4_BASE + 0x68U)
#define	PFC_PUEN_GP4_DM0		(PFC_GP4_BASE + 0xC0U)
#define	PFC_PUD_GP4_DM0			(PFC_GP4_BASE + 0xE0U)
#define	PFC_DRVCTRL0_GP4_DM0		(PFC_GP4_BASE + 0x80U)
#define	PFC_DRVCTRL1_GP4_DM0		(PFC_GP4_BASE + 0x84U)
#define	PFC_POC_GP4_DM0			(PFC_GP4_BASE + 0xA0U)

#define	GPIO_GP4_DM0_BASE_ADDRESS	0xE6060100
#define	GPIO_INOUTSEL_GP4_DM0		(GPIO_GP4_DM0_BASE_ADDRESS + 0x84U)
#define	GPIO_OUTDT_GP4_DM0		(GPIO_GP4_DM0_BASE_ADDRESS + 0x88U)

void init_pfc_avb(void)
{
	u32 dataL;

	/*
	 * IP0SR4[31:0] = 32'h0 = AVB0_TXC, AVB0_TX_CT, AVB0_RD3, AVB0_RD2,
	 *			AVB0_RD1, AVB0_RD0, AVB0_RXC, AVB0_RX_CT
	 * IP*SRn * 0 to 3
	 */
	dataL = readl(PFC_IPSR0_GP4_DM0);
	dataL &= ~(0xffffffff);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_IPSR0_GP4_DM0);

	/*
	 * GPSR4[7:0] = 8'b11111111 = AVB0_TXC, AVB0_TX_CT, AVB0_RD3, AVB0_RD2,
	 *			AVB0_RD1, AVB0_RD0, AVB0_RXC, AVB0_RX_CT
	 */
	dataL = readl(PFC_GPSR_GP4_DM0);
	dataL &= ~(0xff);
	dataL |= (0xff);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_GPSR_GP4_DM0);

	/*
	 * IP1SR4[31:0] = 32'h0 = GP4_15(Output,AVB0_MAGIC), AVB0_MDC,
	 *			AVB0_MDIO, AVB0_TXCREFCLK, AVB0_TD3, AVB0_TD2,
	 *			AVB0_TD1, AVB0_TD0,
	 */
	dataL = readl(PFC_IPSR1_GP4_DM0);
	dataL &= ~(0xffffffff);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_IPSR1_GP4_DM0);

	/*
	 * GPSR4[15:8] = 8'b01111111 = GP4_15(Output,AVB0_MAGIC), AVB0_MDC,
	 *			AVB0_MDIO, AVB0_TXCREFCLK, AVB0_TD3, AVB0_TD2,
	 *			AVB0_TD1, AVB0_TD0,
	 */
	dataL = readl(PFC_GPSR_GP4_DM0);
	dataL &= ~(0xff00);
	dataL |= (0x7f00);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_GPSR_GP4_DM0);

	/* GP4_15 = AVB0_RESETn_V, GPIO-Output Initial-High */
	/* Initial-High */
	writel(readl(GPIO_OUTDT_GP4_DM0) | BIT(15), GPIO_OUTDT_GP4_DM0);
	/* 0=Input / 1=Output */
	writel(readl(GPIO_INOUTSEL_GP4_DM0) | BIT(15), GPIO_INOUTSEL_GP4_DM0);

	/*
	 * IP2SR4[19:4] = 16'h0 = No-Setting, GP4_20(Input,AVB0_AVTP_PPS),
	 *		GP4_19(Input,AVB0_AVTP_CAPTURE),
	 *		GP4_18(Input,AVB0_AVTP_MATCH), AVB0_LINK, AVB0_PHY_INT
	 */
	dataL = readl(PFC_IPSR2_GP4_DM0);
	dataL &= ~(0xffff0);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_IPSR2_GP4_DM0);

	/*
	 * GPSR4[20:16] = 5'b00011 = GP4_20(Input,AVB0_AVTP_PPS),
	 *		GP4_19(Input,AVB0_AVTP_CAPTURE),
	 *		GP4_18(Input,AVB0_AVTP_MATCH), AVB0_LINK, AVB0_PHY_INT
	 */
	dataL = readl(PFC_GPSR_GP4_DM0);
	dataL &= ~(0x1f0000);
	dataL |= (0x30000);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_GPSR_GP4_DM0);

	/* PUEN4[6] = AVB_TX_CTL is 1. PUEN4[20:7][5:0] = set to 0. */
	dataL = readl(PFC_PUEN_GP4_DM0);
	dataL &= ~(0x1fffff); /* 0=Disable Pull-Up/Down */
	dataL |= BIT(6);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_PUEN_GP4_DM0);

	/*
	 * PUD4[6] = PUD_AVB_TX_CTL -> 1: Pull-up is enabled.
	 * PUD4[5:0] = PUD_AVB_RD[3:0], PUD_AVB_RXC,
	 * PUD_AVB_RX_CTL -> 0: Pull-down is enabled.
	 */
	dataL = readl(PFC_PUD_GP4_DM0);
	dataL &= ~(0x7f);
	dataL |= BIT(6);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_PUD_GP4_DM0);

	/*
	 * DRV0CTRL4	[31:28]	AVB0_TXC
	 *			[27:24]	AVB0_TX_CTL
	 * DRV1CTRL4	[15:12]	AVB0_TD3
	 *			[11:8]	AVB0_TD2
	 *			[7:4]	AVB0_TD1
	 *			[3:0]	AVB0_TD0
	 */
	dataL = readl(PFC_DRVCTRL0_GP4_DM0);
	dataL &= ~0x77000000;
	dataL |= 0x33000000;
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_DRVCTRL0_GP4_DM0);

	dataL = readl(PFC_DRVCTRL1_GP4_DM0);
	dataL &= ~0x00007777;
	dataL |= 0x00003333;
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_DRVCTRL1_GP4_DM0);

	/*
	 * Ether AVB0 IO voltage level = 2.5V
	 * POC4 bit[17:0] = 0 = IO voltage level = 2.5V
	 */
	dataL = readl(PFC_POC_GP4_DM0);
	dataL &= ~(0x3ffff);
	writel(~dataL, PFC_PMMR);
	writel(dataL, PFC_POC_GP4_DM0);
}

int board_early_init_f(void)
{
	/* Unlock CPG access */
	writel(0x5A5AFFFF, CPGWPR);
	writel(0xA5A50000, CPGWPCR);

	init_pfc_avb();
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_TEXT_BASE + 0x50000;

	init_gic_v3();

	return 0;
}

#define RST_BASE	0xE6160000 /* Domain0 */
#define RST_WDTRSTCR	(RST_BASE + 0x10)
#define RST_WDT_CODE	0xA55A0002

void reset_cpu(ulong addr)
{
	writel(RST_WDT_CODE, RST_WDTRSTCR);
}
