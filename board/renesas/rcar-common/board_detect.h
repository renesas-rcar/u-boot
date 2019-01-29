/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Renesas Electronics Corporation
 */

#define BOARD_ID_SALVATOR_X	0x00
#define BOARD_ID_SK_PRO		0x02
#define BOARD_ID_SALVATOR_XS	0x04
#define BOARD_ID_EBISU		0x08
#define BOARD_ID_SK_PREM	0x0b
#define BOARD_ID_EBISU_4D	0x0d
#define BOARD_ID_UNKNOWN	0xff
#define BOARD_REV_UNKNOWN	0xff

struct rcar_dram_conf_t {
	u64 dram_type;
	u64 legacy_address;
	u64 legacy_size;
	u64 address[4];
	u64 size[4];
};

struct rcar_dt_fit_t {
	unsigned char board_id;
	unsigned char board_rev;
	const char *target_name;
};

int32_t board_detect_type(struct rcar_dt_fit_t *dt_fit);
int32_t board_detect_dram(struct rcar_dram_conf_t *dram_conf_addr);
