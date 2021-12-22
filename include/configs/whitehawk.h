/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/whitehawk.h
 *     This file is White Hawk board configuration.
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 */

#ifndef __WHITEHAWK_H
#define __WHITEHAWK_H

#include "rcar-gen4-common.h"

/* Ethernet RAVB */
#define CONFIG_BITBANGMII_MULTI

/* Board Clock */
/* XTAL_CLK : 16.66MHz */
#define CONFIG_SYS_CLK_FREQ	16666666u

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0xFE502A	/* 16.66MHz from CPclk */

#endif /* __WHITEHAWK_H */

