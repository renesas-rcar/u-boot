/*
 * arch/arm/include/asm/arch-rcar_gen3/ehci-rcar.h
 *	This file is USB-related definitions.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __EHCI_RCAR_H__
#define __EHCI_RCAR_H__

/* Register offset */
#define OHCI_OFFSET	0x00
#define OHCI_SIZE	0x100
#define EHCI_OFFSET	0x100		/* offset: 0100H */
#define EHCI_SIZE	0x100
#define AHB_OFFSET	0x200
#define USB_CORE_OFFSET	0x300

#define EHCI_USBCMD	(EHCI_OFFSET + 0x0020)

/* USBCTR */
#define DIRPD		(1 << 2)
#define PLL_RST		(1 << 1)

/* INT_ENABLE(offset:200H) */
#define USBH_INTBEN		(1 << 2)
#define USBH_INTAEN		(1 << 1)

/* AHB_BUS_CTR */				/* hw default is .. */
#define PROT_TYPE_CACHEABLE	(1 << 15)	/* 0: non-cacheable trns */
#define PROT_TYPE_BUFFERABLE	(1 << 14)	/* 0: non-bufferable trns */
#define PROT_TYPE_PRIVILEGED	(1 << 13)	/* 0: user access */
#define PROT_TYPE_DATA		(1 << 12)	/* 0: opcode */
#define PROT_MODE		(1 << 8)
#define ALIGN_ADDRESS_1K	(0 << 4)	/* h/w default */
#define ALIGN_ADDRESS_64BYTE	(1 << 4)
#define ALIGN_ADDRESS_32BYTE	(2 << 4)
#define ALIGN_ADDRESS_16BYTE	(3 << 4)
#define MAX_BURST_LEN_INCR16	(0 << 0)	/* h/w default */
#define MAX_BURST_LEN_INCR8	(1 << 0)
#define MAX_BURST_LEN_INCR4	(2 << 0)
#define MAX_BURST_LEN_SINGLE	(3 << 0)

#define SMSTPCR7	0xE615014C
#define SMSTPCR701	(1 << 1)	/* EHCI2 */
#define SMSTPCR702	(1 << 2)	/* EHCI1 */
#define SMSTPCR703	(1 << 3)	/* EHCI0 */
#define SMSTPCR704	(1 << 4)	/* HSUSB */

/* Init AHB master and slave functions of the host logic */
#define AHB_BUS_CTR_INIT 0

struct ahb_bridge {
	u32 int_enable;
	u32 int_status;
	u32 ahb_bus_ctr;
	u32 usbctr;
};

struct rmobile_ehci_reg {
	u32 hciversion;		/* hciversion/caplength */
	u32 hcsparams;		/* hcsparams */
	u32 hccparams;		/* hccparams */
	u32 hcsp_portroute;	/* hcsp_portroute */
	u32 usbcmd;		/* usbcmd */
	u32 usbsts;		/* usbsts */
	u32 usbintr;		/* usbintr */
	u32 frindex;		/* frindex */
	u32 ctrldssegment;	/* ctrldssegment */
	u32 periodiclistbase;	/* periodiclistbase */
	u32 asynclistaddr;	/* asynclistaddr */
	u32 dummy[9];
	u32 configflag;		/* configflag */
	u32 portsc;		/* portsc */
};

struct usb_core_reg {
	u32 revid;
	u32 regen_cg_ctrl;
	u32 spd_ctrl;
	u32 spd_rsm_timset;
	u32 oc_timset;
	u32 sbrn_fladj_pw;
};

#endif /* __EHCI_RCAR_H__ */
