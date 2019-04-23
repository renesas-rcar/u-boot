/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/configs/v3hsk.h
 *     This file is V3HSK board configuration.
 *
 * Copyright (C) 2019 Renesas Electronics Corporation
 * Copyright (C) 2019 Cogent Embedded, Inc.
 */

#ifndef __V3HSK_H
#define __V3HSK_H

#include "rcar-gen3-common.h"

/* Ethernet */
#define CONFIG_BITBANGMII_MULTI

/* Environment compatibility */

/* SH Ether */
#ifdef CONFIG_SH_ETHER
#define CONFIG_SH_ETHER_USE_PORT	0
#define CONFIG_SH_ETHER_PHY_ADDR	0x0
#define CONFIG_SH_ETHER_PHY_MODE	PHY_INTERFACE_MODE_RGMII_ID

#define CONFIG_SH_ETHER_CACHE_WRITEBACK
#define CONFIG_SH_ETHER_CACHE_INVALIDATE
#define CONFIG_SH_ETHER_ALIGNE_SIZE	64
#endif

/* Board Clock */
/* XTAL_CLK : 33.33MHz */
#define CONFIG_SYS_CLK_FREQ	33333333u

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0xFE502A	/* 16.66MHz from CPclk */

#endif /* __V3HSK_H */
