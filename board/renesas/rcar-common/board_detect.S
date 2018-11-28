/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2019 Renesas Electronics Corporation
 */

#include <config.h>
#include <linux/linkage.h>

/*
 * int32_t board_detect_type(rcar_dt_fit_t *dt_fit);
 */
ENTRY(board_detect_type)
	movz	w3, #0x1
	mov	x4, x0
	movk	w3, #0x8200, lsl #16
	mov	x0, x3
	smc	#0
	cbnz	w0, #_board_no_detect_type
	strb	w1, [x4]
	strb	w2, [x4, #1]
_board_detect_type_end:
	ret
_board_no_detect_type:
	movn	w1, #0
	strb	w1, [x4]
	b	#_board_detect_type_end
ENDPROC(board_detect_type)

/*
 * int32_t board_detect_dram(rcar_dram_conf_t *dram_conf_addr);
 */
ENTRY(board_detect_dram)
	mov	x1, x0
	movz	w0, #0x8200, lsl #16
	smc	#0
	ret
ENDPROC(board_detect_dram)
	.ltorg
