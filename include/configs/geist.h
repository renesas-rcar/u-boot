/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * include/configs/geist.h
 *     This file is Geist board configuration.
 *
 * Copyright (C) 2025 Renesas Electronics Corporation
 */

#ifndef __GEIST_H
#define __GEIST_H

#include "rcar-gen3-common.h"

/* Ethernet RAVB */
#define CONFIG_BITBANGMII_MULTI

/* Generic Timer Definitions (use in assembler source) */
#define COUNTER_FREQUENCY	0xFE502A	/* 16.66MHz from CPclk */

/* Environment in eMMC, at the end of 2nd "boot sector" */

#endif /* __GEIST_H */
