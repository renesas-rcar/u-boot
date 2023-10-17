/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/rcar-x5h.h
 *     This file is R-Car X5H board configuration.
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#ifndef __RCAR_X5H_BOARD_H
#define __RCAR_X5H_BOARD_H

#include "rcar-gen5-common.h"

/* Board Clock */
/* XTAL_CLK : 16.66MHz */
#define CONFIG_SYS_CLK_FREQ	16666666u

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0xFE502A	/* 16.66MHz from CPclk */

#endif /* __RCAR_X5H_BOARD_H */

