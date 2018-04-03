/*
 * arch/arm/cpu/armv8/rcar_gen3/board.c
 * This file is a description of a function that depends on
 * the version of the product.
 *
 * Copyright (C) 2017-2018 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/prr_depend.h>

#define MODEMR		(0xE6160060ul) /* Mode Monitor Register */
#define MD12MASK	(0x00001000ul) /* MD12(bit12) MASK */

DECLARE_GLOBAL_DATA_PTR;

void board_cleanup_preboot_os(void)
		__attribute__((weak, alias("__board_cleanup_preboot_os")));

static void __board_cleanup_preboot_os(void)
{
	return;
}

int rcar_get_serial_config_clk(void)
{
#if defined(CONFIG_TARGET_DRAAK)
	u32 rcar_modemr = readl(MODEMR);
	if (rcar_modemr & MD12MASK)
		return CONFIG_SH_SCIF_SSCG_CLK_FREQ;
	else
		return CONFIG_SH_SCIF_CLEAN_CLK_FREQ;
#elif defined(CONFIG_TARGET_EBISU)
	u32 rcar_modemr = readl(MODEMR);
	if (rcar_modemr & MD12MASK)
		return CONFIG_SH_SCIF_SSCG_CLK_FREQ;
	else
		return CONFIG_SH_SCIF_CLEAN_CLK_FREQ;
#else
#if defined(CONFIG_R8A7795) || defined(CONFIG_R8A7796X)
	return rcar_get_serial_config_clk_prr();
#else
#error "Not found serial clock configuration."
#endif
#endif
}

/* Always output ram info at R-Car Gen3 series */
void board_add_ram_info(int use_default)
{
	int i;

	printf("\n");
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		printf("Bank #%d: 0x%09llx - 0x%09llx, ", i,
		       (unsigned long long)(gd->bd->bi_dram[i].start),
		       (unsigned long long)(gd->bd->bi_dram[i].start
		       + gd->bd->bi_dram[i].size - 1));
		print_size(gd->bd->bi_dram[i].size, "\n");
	}
}
