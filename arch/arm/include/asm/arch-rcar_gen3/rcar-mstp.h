/*
 * arch/arm/include/asm/arch-rcar_gen3/rcar-mstp.h
 *	This file defines lowlevel functions for MSTP registers.
 *
 * Copyright (C) 2015-2018 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARCH_RCAR_MSTP_H
#define __ASM_ARCH_RCAR_MSTP_H

#define mstp_setbits(type, addr, saddr, set) \
		out_##type((saddr), in_##type(addr) | (set))
#define mstp_clrbits(type, addr, saddr, clear) \
		out_##type((saddr), in_##type(addr) & ~(clear))
#define mstp_setclrbits(type, addr, set, clear) \
		out_##type((addr), (in_##type(addr) | (set)) & ~(clear))
#define mstp_setbits_le32(addr, saddr, set) \
		mstp_setbits(le32, addr, saddr, set)
#define mstp_clrbits_le32(addr, saddr, clear) \
		mstp_clrbits(le32, addr, saddr, clear)
#define mstp_setclrbits_le32(addr, set, clear) \
		mstp_setclrbits(le32, addr, set, clear)

#define mstp_wait_clrbits(type, reg, clear)	\
({						\
	unsigned int timeout = 10000;		\
	int ret = true;				\
						\
	while (in_##type(reg) & (clear)) {	\
		timeout--;			\
		udelay(10);			\
		if (timeout == 0) {		\
			/* false, but not yet init print */ \
			ret = false;		\
			break;			\
		}				\
	}					\
	ret;					\
});

#define mstp_wait_clrbits_le32(reg, clear) \
	mstp_wait_clrbits(le32, reg, clear)



#endif /* __ASM_ARCH_RCAR_MSTP_H */
