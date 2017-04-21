/*
 * board/renesas/rcar-gen3-common/common.c
 *	This file defines common function for R-Car Gen3
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/rcar_gen3.h>
#include <asm/arch/rcar-mstp.h>

void arch_preboot_os(void)
{
	board_cleanup_preboot_os();
}
