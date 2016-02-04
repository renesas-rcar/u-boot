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
#define PRR_PRODUCT_MASK		(0x7f00ul)

/* PRR PRODUCT and CUT for RCAR */
#define PRR_PRODUCT_CUT_RCAR_H3_WS10	(PRR_PRODUCT_RCAR_H3   | 0x00ul)
#define PRR_PRODUCT_CUT_RCAR_H3_WS11	(PRR_PRODUCT_RCAR_H3   | 0x01ul)
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

