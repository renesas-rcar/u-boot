/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/spider.h
 *     This file is Spider board configuration.
 *
 * Copyright (C) 2021 Renesas Electronics Corp.
 */

#ifndef __SPIDER_H
#define __SPIDER_H

#include "rcar-gen4-common.h"

/* Board Clock */
/* XTAL_CLK : 20.00MHz */
#define CONFIG_SYS_CLK_FREQ	20000000u

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0x1312D00	/* 20.00MHz from CPclk */

#endif /* __FALCON_H */
