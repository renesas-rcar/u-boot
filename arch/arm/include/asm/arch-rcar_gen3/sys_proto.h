/*
 * arch/arm/include/asm/arch-rcar_gen3/sys_proto.h
 *	This file defines prototype of system.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

struct rcar_sysinfo {
	char *board_string;
};
extern const struct rcar_sysinfo sysinfo;

#endif
