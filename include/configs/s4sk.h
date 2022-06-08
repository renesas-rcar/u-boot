/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/s4sk.h
 *     This file is S4 Starter Kit board configuration.
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#ifndef __S4SK_H
#define __S4SK_H

#include "rcar-gen4-common.h"

/* Board Clock */
/* XTAL_CLK : 20.00MHz */
#define CONFIG_SYS_CLK_FREQ	20000000u

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0x1312D00	/* 20.00MHz from CPclk */

#endif /* __S4SK_H */
