/*
 * arch/arm/cpu/armv8/rcar_gen3/board.c
 * This file is a description of a function that depends on
 * the version of the product.
 *
 * Copyright (C) 2017 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

void board_cleanup_preboot_os(void)
		__attribute__((weak, alias("__board_cleanup_preboot_os")));

static void __board_cleanup_preboot_os(void)
{
	return;
}
