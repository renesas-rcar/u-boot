/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/v3msk.h
 *     This file is V3MSK board configuration.
 *
 * Copyright (C) 2019 Renesas Electronics Corporation
 * Copyright (C) 2019 Cogent Embedded, Inc.
 */

#ifndef __V3MSK_H
#define __V3MSK_H

#include "rcar-gen3-common.h"

/* Ethernet RAVB */
#define CONFIG_BITBANGMII
#define CONFIG_BITBANGMII_MULTI

/* Board Clock */
/* XTAL_CLK : 33.33MHz */
#define CONFIG_SYS_CLK_FREQ	33333333u

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0xFE502A	/* 16.66MHz from CPclk */

#endif /* __V3MSK_H */
