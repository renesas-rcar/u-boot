/*
 * arch/arm/cpu/armv8/rcar_gen3/prr_depend.c
 * This file is a description of a function that depends on
 * the version of the product.
 *
 * Copyright (C) 2016 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>

#define PRR				(0xfff00044ul) /* Product Register */

/* PRR PRODUCT for RCAR */
#define PRR_PRODUCT_RCAR_H3		(0x4f00ul)
#define PRR_PRODUCT_RCAR_M3		(0x5200ul)
#define PRR_PRODUCT_MASK		(0x7f00ul)

/* PRR PRODUCT and CUT for RCAR */
#define PRR_PRODUCT_CUT_RCAR_H3_WS10	(PRR_PRODUCT_RCAR_H3   | 0x00ul)
#define PRR_PRODUCT_CUT_RCAR_H3_WS11	(PRR_PRODUCT_RCAR_H3   | 0x01ul)
#define PRR_PRODUCT_CUT_RCAR_M3_ES10	(PRR_PRODUCT_RCAR_M3   | 0x00ul)
#define PRR_PRODUCT_CUT_MASK		(PRR_PRODUCT_MASK      | 0xfful)

#define RCAR_PRR_INIT()			rcar_prr_init()

#define RCAR_PRR_IS_PRODUCT(a) \
		rcar_prr_compare_product(PRR_PRODUCT_RCAR_##a)

#define RCAR_PRR_CHK_CUT(a, b) \
		rcar_prr_check_product_cut(PRR_PRODUCT_CUT_RCAR_##a##_##b)

static u32 rcar_prr = 0xffffffff;

static int rcar_prr_compare_product(u32 id)
{
	return (rcar_prr & PRR_PRODUCT_MASK) == (id & PRR_PRODUCT_MASK);
}

static int rcar_prr_check_product_cut(u32 id)
{
	return (rcar_prr & PRR_PRODUCT_CUT_MASK) - (id & PRR_PRODUCT_CUT_MASK);
}

void rcar_prr_init(void)
{
	rcar_prr = readl(PRR);
}

/*
 * for serial function
 */
int rcar_get_serial_config_clk(void)
{
	if (RCAR_PRR_IS_PRODUCT(H3) && (!RCAR_PRR_CHK_CUT(H3, WS10)))
		return CONFIG_SYS_CLK_FREQ;
	else
		return CONFIG_SH_SCIF_CLK_FREQ;
}

/*
 * for sd/mmc function
 */
#define STP_HCK		(1 << 9)
#define SD_SRCFC_DIV1	(0 << 2)
#define SD_SRCFC_DIV2	(1 << 2)
#define SD_SRCFC_DIV4	((2 << 2) | STP_HCK)	/* SDnH stop */
#define SD_SRCFC_DIV8	((3 << 2) | STP_HCK)	/* SDnH stop */
#define SD_SRCFC_DIV16	((4 << 2) | STP_HCK)	/* SDnH stop */
#define SD_FC_DIV2	(0 << 0)
#define SD_FC_DIV4	(1 << 0)
#define SDH800_SD200	(SD_SRCFC_DIV1 | SD_FC_DIV4)
#define SDH400_SD200	(SD_SRCFC_DIV1 | SD_FC_DIV2)

int rcar_get_sdhi_config_clk(void)
{
	if (RCAR_PRR_IS_PRODUCT(H3) && (!RCAR_PRR_CHK_CUT(H3, WS10)))
		return SDH400_SD200;
	else
		return SDH800_SD200;
}
