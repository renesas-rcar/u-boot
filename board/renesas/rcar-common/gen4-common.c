// SPDX-License-Identifier: GPL-2.0
/*
 * board/renesas/rcar-common/gen4-common.c
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 */

#include <common.h>
#include <dm.h>
#include <init.h>
#include <dm/uclass-internal.h>
#include <asm/arch/rmobile.h>
#include <linux/libfdt.h>

#ifdef CONFIG_RCAR_GEN4

DECLARE_GLOBAL_DATA_PTR;

/*
 * If the firmware passed a device tree use it for U-Boot DRAM setup
 * and board identification
 */
extern u64 rcar_atf_boot_args[];

int fdtdec_board_setup(const void *fdt_blob)
{
	void *atf_fdt_blob = (void *)(rcar_atf_boot_args[1]);

	if (fdt_magic(atf_fdt_blob) == FDT_MAGIC)
		fdt_overlay_apply_node((void *)fdt_blob, 0, atf_fdt_blob, 0);

	return 0;
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	fdtdec_setup_memory_banksize();

	return 0;
}

void board_add_ram_info(int use_default)
{
	int i;

	printf("\nRAM Configuration:\n");
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
#endif
