/*
 * drivers/net/ravb.h
 *     This file is driver for Renesas Ethernet AVB.
 *
 * Copyright (C) 2015  Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <netdev.h>
#include <asm/types.h>

#define CARDNAME "ravb"

#define NUM_TX_DESC	8
#define NUM_RX_DESC	64
#define RAVB_ALIGN	128

/* Buffers must be big enough to hold the largest ethernet frame. Also, rx
   buffers must be a multiple of 128 bytes */
#define MAX_BUF_SIZE	(RAVB_ALIGN * 12)

/* The ethernet avb descriptor definitions. */
enum DT {
	/* frame data */
	DT_FSTART    = 5,
	DT_FMID      = 4,
	DT_FEND      = 6,
	DT_FSINGLE   = 7,
	/* chain control */
	DT_LINK      = 8,
	DT_LINKFIX   = 9,
	DT_EOS       = 10,
	/* HW/SW arbitration */
	DT_FEMPTY    = 12,
	DT_FEMPTY_IS = 13,
	DT_FEMPTY_IC = 14,
	DT_FEMPTY_ND = 15,
	DT_LEMPTY    = 2,
	DT_EEMPTY    = 3,
	/* 0,1,11 is reserved */
};

struct ravb_desc {
#if defined(__LITTLE_ENDIAN)
	volatile u32 ds:12;	/* descriptor size */
	volatile u32 cc:12;	/* content control */
	volatile u32 die:4;	/* descriptor interrupt enable */
			/* 0:disable, other:enable */
	volatile u32 dt:4;	/* dscriotor type */
#else
	volatile u32 dt:4;	/* dscriotor type */
	volatile u32 die:4;	/* descriptor interrupt enable */
			/* 0:disable, other:enable */
	volatile u32 cc:12;	/* content control */
	volatile u32 ds:12;	/* descriptor size */
#endif
	volatile u32 dptr;	/* descpriptor pointer */
};

/* MAC reception status */
enum MSC {
	MSC_MC   = 1<<7, /* [7] Multicast frame reception */
	MSC_CEEF = 1<<6, /* [6] Carrier extension error */
	MSC_CRL  = 1<<5, /* [5] Carrier lost */
	MSC_FRE  = 1<<4, /* [4] Fraction error (isn't a multiple of 8bits) */
	MSC_RTLF = 1<<3, /* [3] Frame length error (frame too long) */
	MSC_RTSF = 1<<2, /* [2] Frame length error (frame too short) */
	MSC_RFE  = 1<<1, /* [1] Frame reception error (flagged by PHY) */
	MSC_CRC  = 1<<0, /* [0] Frame CRC error */
};

#define MSC_RX_ERR_MASK (MSC_CRC | MSC_RFE | MSC_RTLF | MSC_RTSF | MSC_CEEF)

struct ravb_txdesc {
#if defined(__LITTLE_ENDIAN)
	volatile u32 ds:12;	/* descriptor size */
	volatile u32 tag:10;	/* frame tag */
	volatile u32 tsr:1;	/* timestamp storeage request */
	volatile u32 msc:1;	/* mac status storeage request */
	volatile u32 die:4;	/* descriptor interrupt enable */
			/* 0:disable, other:enable */
	volatile u32 dt:4;	/* dscriotor type */
#else
	volatile u32 dt:4;	/* dscriotor type */
	volatile u32 die:4;	/* descriptor interrupt enable */
			/* 0:disable, other:enable */
	volatile u32 msc:1;	/* mac status storeage request */
	volatile u32 tsr:1;	/* timestamp storeage request */
	volatile u32 tag:10;	/* frame tag */
	volatile u32 ds:12;	/* descriptor size */
#endif
	volatile u32 dptr;	/* descpriptor pointer */
};

struct ravb_rxdesc {
#if defined(__LITTLE_ENDIAN)
	volatile u32 ds:12;	/* descriptor size */
	volatile u32 ei:1;	/* error indication */
	volatile u32 ps:2;	/* padding selection */
	volatile u32 tr:1;	/* truncation indication */
	volatile u32 msc:8;	/* mac status code */
	volatile u32 die:4;	/* descriptor interrupt enable */
			/* 0:disable, other:enable */
	volatile u32 dt:4;	/* dscriotor type */
#else
	volatile u32 dt:4;	/* dscriotor type */
	volatile u32 die:4;	/* descriptor interrupt enable */
			/* 0:disable, other:enable */
	volatile u32 msc:8;	/* mac status code */
	volatile u32 ps:2;	/* padding selection */
	volatile u32 ei:1;	/* error indication */
	volatile u32 tr:1;	/* truncation indication */
	volatile u32 ds:12;	/* descriptor size */
#endif
	volatile u32 dptr;	/* descpriptor pointer */
};

#define DBAT_ENTRY_NUM	(22)
#define TX_QUEUE	(0)
#define RX_QUEUE	(4)

#define RFLR_RFL_MIN	0x05EE	/* Recv Frame length 1518 byte */

struct ravb_dev {
	struct ravb_desc *desc_bat_alloc;
	struct ravb_desc *desc_bat_base;
	struct ravb_txdesc *tx_desc_alloc;
	struct ravb_txdesc *tx_desc_base;
	struct ravb_txdesc *tx_desc_cur;
	struct ravb_rxdesc *rx_desc_alloc;
	struct ravb_rxdesc *rx_desc_base;
	struct ravb_rxdesc *rx_desc_cur;
	u8 *rx_buf_alloc;
	u8 *rx_buf_base;
	u8 mac_addr[6];
	u8 phy_addr;
	struct eth_device *dev;
	struct phy_device *phydev;
};

/* from linux/drivers/net/ethernet/renesas/ravb.h */
enum {
	/* AVB-DMAC specific registers */
	CCC,
	DBAT,
	DLR,
	CSR,
	CDAR0,
	CDAR1,
	CDAR2,
	CDAR3,
	CDAR4,
	CDAR5,
	CDAR6,
	CDAR7,
	CDAR8,
	CDAR9,
	CDAR10,
	CDAR11,
	CDAR12,
	CDAR13,
	CDAR14,
	CDAR15,
	CDAR16,
	CDAR17,
	CDAR18,
	CDAR19,
	CDAR20,
	CDAR21,
	ESR,
	APSR,
	RCR,
	RQC,
	RPC,
	UFCW,
	UFCS,
	UFCV0,
	UFCV1,
	UFCV2,
	UFCV3,
	UFCV4,
	UFCD0,
	UFCD1,
	UFCD2,
	UFCD3,
	UFCD4,
	SFO,
	SFP0,
	SFP1,
	SFP2,
	SFP3,
	SFP4,
	SFP5,
	SFP6,
	SFP7,
	SFP8,
	SFP9,
	SFP10,
	SFP11,
	SFP12,
	SFP13,
	SFP14,
	SFP15,
	SFP16,
	SFP17,
	SFP18,
	SFP19,
	SFP20,
	SFP21,
	SFP22,
	SFP23,
	SFP24,
	SFP25,
	SFP26,
	SFP27,
	SFP28,
	SFP29,
	SFP30,
	SFP31,
	SFM0,
	SFM1,
	RTSR,
	CIAR,
	LIAR,
	TGC,
	TCCR,
	TSR,
	MFA,
	TFA0,
	TFA1,
	TFA2,
	CIVR0,
	CIVR1,
	CDVR0,
	CDVR1,
	CUL0,
	CUL1,
	CLL0,
	CLL1,
	DIC,
	DIS,
	EIC,
	EIS,
	RIC0,
	RIS0,
	RIC1,
	RIS1,
	RIC2,
	RIS2,
	TIC,
	TIS,
	ISS,
	GCCR,
	GMTT,
	GPTC,
	GTI,
	GTO0,
	GTO1,
	GTO2,
	GIC,
	GIS,
	GCPT,
	GCT0,
	GCT1,
	GCT2,

	/* Ether registers */
	ECMR,
	ECSR,
	ECSIPR,
	PIR,
	PSR,
	RDMLR,
	PIPR,
	RFLR,
	IPGR,
	APR,
	MPR,
	PFTCR,
	PFRCR,
	RFCR,
	RFCF,
	TPAUSER,
	TPAUSECR,
	BCFR,
	BCFRR,
	GECMR,
	BCULR,
	MAHR,
	MALR,
	TROCR,
	CDCR,
	LCCR,
	CNDCR,
	CEFCR,
	FRECR,
	TSFRCR,
	TLFRCR,
	CERCR,
	CEECR,
	MAFCR,
	RTRATE,
	CSMR,

	/* This value must be written at last. */
	RAVB_MAX_REGISTER_OFFSET,
};

static const u16 ravb_offset_rcar_gen2[RAVB_MAX_REGISTER_OFFSET] = {
	/* AVB-DMAC registers */
	[CCC] = 0x0000,
	[DBAT] = 0x0004,
	[DLR] = 0x0008,
	[CSR] = 0x000C,
	[CDAR0] = 0x0010,
	[CDAR1] = 0x0014,
	[CDAR2] = 0x0018,
	[CDAR3] = 0x001C,
	[CDAR4] = 0x0020,
	[CDAR5] = 0x0024,
	[CDAR6] = 0x0028,
	[CDAR7] = 0x002C,
	[CDAR8] = 0x0030,
	[CDAR9] = 0x0034,
	[CDAR10] = 0x0038,
	[CDAR11] = 0x003C,
	[CDAR12] = 0x0040,
	[CDAR13] = 0x0044,
	[CDAR14] = 0x0048,
	[CDAR15] = 0x004C,
	[CDAR16] = 0x0050,
	[CDAR17] = 0x0054,
	[CDAR18] = 0x0058,
	[CDAR19] = 0x005C,
	[CDAR20] = 0x0060,
	[CDAR21] = 0x0064,
	[ESR] = 0x0088,
	[APSR] = 0x008C,
	[RCR] = 0x0090,
	[RQC] = 0x0094,
	[RPC] = 0x00B0,
	[UFCW] = 0x00BC,
	[UFCS] = 0x00C0,
	[UFCV0] = 0x00C4,
	[UFCV1] = 0x00C8,
	[UFCV2] = 0x00CC,
	[UFCV3] = 0x00D0,
	[UFCV4] = 0x00D4,
	[UFCD0] = 0x00E0,
	[UFCD1] = 0x00E4,
	[UFCD2] = 0x00E8,
	[UFCD3] = 0x00EC,
	[UFCD4] = 0x00F0,
	[SFO] = 0x00FC,
	[SFP0] = 0x0100,
	[SFP1] = 0x0104,
	[SFP2] = 0x0108,
	[SFP3] = 0x010c,
	[SFP4] = 0x0110,
	[SFP5] = 0x0114,
	[SFP6] = 0x0118,
	[SFP7] = 0x011c,
	[SFP8] = 0x0120,
	[SFP9] = 0x0124,
	[SFP10] = 0x0128,
	[SFP11] = 0x012c,
	[SFP12] = 0x0130,
	[SFP13] = 0x0134,
	[SFP14] = 0x0138,
	[SFP15] = 0x013c,
	[SFP16] = 0x0140,
	[SFP17] = 0x0144,
	[SFP18] = 0x0148,
	[SFP19] = 0x014c,
	[SFP20] = 0x0150,
	[SFP21] = 0x0154,
	[SFP22] = 0x0158,
	[SFP23] = 0x015c,
	[SFP24] = 0x0160,
	[SFP25] = 0x0164,
	[SFP26] = 0x0168,
	[SFP27] = 0x016c,
	[SFP28] = 0x0170,
	[SFP29] = 0x0174,
	[SFP30] = 0x0178,
	[SFP31] = 0x017c,
	[SFM0] = 0x01C0,
	[SFM1] = 0x01C4,
	[RTSR] = 0x01D0,
	[CIAR] = 0x0200,
	[LIAR] = 0x0280,
	[TGC] = 0x0300,
	[TCCR] = 0x0304,
	[TSR] = 0x0308,
	[MFA] = 0x030C,
	[TFA0] = 0x0310,
	[TFA1] = 0x0314,
	[TFA2] = 0x0318,
	[CIVR0] = 0x0320,
	[CIVR1] = 0x0324,
	[CDVR0] = 0x0328,
	[CDVR1] = 0x032C,
	[CUL0] = 0x0330,
	[CUL1] = 0x0334,
	[CLL0] = 0x0338,
	[CLL1] = 0x033C,
	[DIC] = 0x0350,
	[DIS] = 0x0354,
	[EIC] = 0x0358,
	[EIS] = 0x035C,
	[RIC0] = 0x0360,
	[RIS0] = 0x0364,
	[RIC1] = 0x0368,
	[RIS1] = 0x036C,
	[RIC2] = 0x0370,
	[RIS2] = 0x0374,
	[TIC] = 0x0378,
	[TIS] = 0x037C,
	[ISS] = 0x0380,
	[GCCR] = 0x0390,
	[GMTT] 0x0394,
	[GPTC] = 0x0398,
	[GTI] = 0x039C,
	[GTO0] = 0x03A0,
	[GTO1] = 0x03A4,
	[GTO2] = 0x03A8,
	[GIC] = 0x03AC,
	[GIS] = 0x03B0,
	[GCPT] = 0x03B4,
	[GCT0] = 0x03B8,
	[GCT1] = 0x03BC,
	[GCT2] = 0x03C0,

	[ECMR]	= 0x0500,
	[ECSR]	= 0x0510,
	[ECSIPR]	= 0x0518,
	[PIR]	= 0x0520,
	[PSR]	= 0x0528,
	[PIPR]	= 0x052c,
	[RFLR]	= 0x0508,
	[APR]	= 0x0554,
	[MPR]	= 0x0558,
	[PFTCR]	= 0x055c,
	[PFRCR]	= 0x0560,
	[TPAUSER]	= 0x0564,
	[GECMR]	= 0x05b0,
	[BCULR]	= 0x05b4,
	[MAHR]	= 0x05c0,
	[MALR]	= 0x05c8,
	[TROCR]	= 0x0700,
	[CDCR]	= 0x0708,
	[LCCR]	= 0x0710,
	[CEFCR]	= 0x0740,
	[FRECR]	= 0x0748,
	[TSFRCR]	= 0x0750,
	[TLFRCR]	= 0x0758,
	[RFCR]	= 0x0760,
	[CERCR]	= 0x0768,
	[CEECR]	= 0x0770,
	[MAFCR]	= 0x0778,
};

/* Register Address */
#define BASE_IO_ADDR	0xE6800000

/* Register's bits of Ethernet AVB */
/* CCC */
enum CCC_BIT {
	CCC_OPC		= 0x00000003,
	CCC_OPC_RESET	= 0x00000000,
	CCC_OPC_CONFIG	= 0x00000001,
	CCC_OPC_OPERATION = 0x00000002,
	CCC_DTSR	= 0x00000100,
	CCC_CSEL	= 0x00030000,
	CCC_CSEL_HPB	= 0x00010000,
	CCC_CSEL_ETH_TX	= 0x00020000,
	CCC_CSEL_GMII_REF = 0x00030000,
	CCC_BOC		= 0x00100000,
	CCC_LBME	= 0x01000000,
};

/* CSR */
enum CSR_BIT {
	CSR_OPS		= 0x0000000F,
	CSR_OPS_RESET	= 0x00000001,
	CSR_OPS_CONFIG	= 0x00000002,
	CSR_OPS_OPERATION = 0x00000004,
	CSR_DTS		= 0x00000100,
	CSR_TPO0	= 0x00010000,
	CSR_TPO1	= 0x00020000,
	CSR_TPO2	= 0x00040000,
	CSR_TPO3	= 0x00080000,
	CSR_RPO		= 0x00100000,
};

enum ESR_BIT {
	ESR_EQN	= 0x0000000f,
	ESR_ET	= 0x00000f00,
	ESR_EIL	= 0x00001000,
};

enum RCR_BIT {
	RCR_EFFS	= 0x00000001,
	RCR_ENCF	= 0x00000002,
	RCR_ESF	= 0x0000000c,
	RCR_ETS0	= 0x00000010,
	RCR_ETS2	= 0x00000020,
	RCR_RFCL	= 0x1fff0000,
};

enum RQC_BIT {
	RQC_RSM0	= 0x00000003,
	TSEL0	= 0x00000004,
	UFCC0	= 0x00000030,
	RQC_RSM1	= 0x00000300,
	TSEL1	= 0x00000400,
	UFCC1	= 0x00003000,
	RQC_RSM2	= 0x00030000,
	TSEL2	= 0x00040000,
	UFCC2	= 0x00300000,
	RQC_RSM3	= 0x03000000,
	TSEL3	= 0x04000000,
	UFCC3	= 0x30000000,
};

enum RPC_BIT {
	RPC_PCNT	= 0x00000700,
	RPC_DCNT	= 0x00ff0000,
};

enum RTC_BIT {
	RTC_MFL0	= 0x00000fff,
	RTC_MFL1	= 0x0fff0000,
};

enum UFCW_BIT {
	UFCW_WL0	= 0x0000003f,
	UFCW_WL1	= 0x00003f00,
	UFCW_WL2	= 0x003f0000,
	UFCW_WL3	= 0x3f000000,
};

enum UFCS_BIT {
	UFCS_SL0	= 0x0000003f,
	UFCS_SL1	= 0x00003f00,
	UFCS_SL2	= 0x003f0000,
	UFCS_SL3	= 0x3f000000,
};

enum UFCV_BIT {
	UFCV_CV0	= 0x0000003f,
	UFCV_CV1	= 0x00003f00,
	UFCV_CV2	= 0x003f0000,
	UFCV_CV3	= 0x3f000000,
};

enum UFCD_BIT {
	UFCD_DV0	= 0x0000003f,
	UFCD_DV1	= 0x00003f00,
	UFCD_DV2	= 0x003f0000,
	UFCD_DV3	= 0x3f000000,
};

enum SFO_BIT {
	SFO_FPB	= 0x0000003f,
};

enum TGC_BIT {
	TGC_TSM0	= 0x00000001,
	TGC_TSM1	= 0x00000002,
	TGC_TSM2	= 0x00000004,
	TGC_TSM3	= 0x00000008,
	TGC_TQP	= 0x00000030,
	TGC_TBD0	= 0x00000300,
	TGC_TBD1	= 0x00003000,
	TGC_TBD2	= 0x00030000,
	TGC_TBD3	= 0x00300000,
};

enum TCCR_BIT {
	TCCR_TSRQ0	= 0x00000001,
	TCCR_TSRQ1	= 0x00000002,
	TCCR_TSRQ2	= 0x00000004,
	TCCR_TSRQ3	= 0x00000008,
	TCCR_TFEN	= 0x00000100,
	TCCR_TFR	= 0x00000200,
	TCCR_MFEN	= 0x00010000,
	TCCR_MFR	= 0x00020000,
};

enum TSR_BIT {
	TSR_CCS0	= 0x00000003,
	TSR_CCS1	= 0x0000000c,
	TSR_TFFL	= 0x00000700,
	TSR_MFFL	= 0x001f0000,
};

enum MFA_BIT {
	MFA_MSV	= 0x0000000f,
	MFA_MST	= 0x03ff0000,
};

enum GCCR_BIT {
	GCCR_TCR	= 0x00000003,
	GCCR_LTO	= 0x00000004,
	GCCR_LTI	= 0x00000008,
	GCCR_LPTC	= 0x00000010,
	GCCR_LMTT	= 0x00000020,
	GCCR_TCSS	= 0x00000300,
};

enum DIC_BIT {
	DIC_DPE1	= 0x00000002,
	DIC_DPE2	= 0x00000004,
	DIC_DPE3	= 0x00000008,
	DIC_DPE4	= 0x00000010,
	DIC_DPE5	= 0x00000020,
	DIC_DPE6	= 0x00000040,
	DIC_DPE7	= 0x00000080,
	DIC_DPE8	= 0x00000100,
	DIC_DPE9	= 0x00000200,
	DIC_DPE10	= 0x00000400,
	DIC_DPE11	= 0x00000800,
	DIC_DPE12	= 0x00001000,
	DIC_DPE13	= 0x00002000,
	DIC_DPE14	= 0x00004000,
	DIC_DPE15	= 0x00008000,
};

enum DIS_BIT {
	DIS_DPF1	= 0x00000002,
	DIS_DPF2	= 0x00000004,
	DIS_DPF3	= 0x00000008,
	DIS_DPF4	= 0x00000010,
	DIS_DPF5	= 0x00000020,
	DIS_DPF6	= 0x00000040,
	DIS_DPF7	= 0x00000080,
	DIS_DPF8	= 0x00000100,
	DIS_DPF9	= 0x00000200,
	DIS_DPF10	= 0x00000400,
	DIS_DPF11	= 0x00000800,
	DIS_DPF12	= 0x00001000,
	DIS_DPF13	= 0x00002000,
	DIS_DPF14	= 0x00004000,
	DIS_DPF15	= 0x00008000,
};

enum EIC_BIT {
	EIC_MREE	= 0x00000001,
	EIC_MTEE	= 0x00000002,
	EIC_QEE	= 0x00000004,
	EIC_SEE	= 0x00000008,
	EIC_CLLE0	= 0x00000010,
	EIC_CLLE1	= 0x00000020,
	EIC_CULE0	= 0x00000040,
	EIC_CULE1	= 0x00000080,
	EIC_TFFE	= 0x00000100,
	EIC_MFFE	= 0x00000200,
};

enum EIS_BIT {
	EIS_MREF	= 0x00000001,
	EIS_MTEF	= 0x00000002,
	EIS_QEF	= 0x00000004,
	EIS_SEF	= 0x00000008,
	EIS_CLLF0	= 0x00000010,
	EIS_CLLF1	= 0x00000020,
	EIS_CULF0	= 0x00000040,
	EIS_CULF1	= 0x00000080,
	EIS_TFFF	= 0x00000100,
	EIS_MFFF	= 0x00000200,
	EIS_QFS	= 0x00010000,
};

enum RIC0_BIT {
	RIC0_FRE0	= 0x00000001,
	RIC0_FRE1	= 0x00000002,
	RIC0_FRE2	= 0x00000004,
	RIC0_FRE3	= 0x00000008,
	RIC0_FRE4	= 0x00000010,
	RIC0_FRE5	= 0x00000020,
	RIC0_FRE6	= 0x00000040,
	RIC0_FRE7	= 0x00000080,
	RIC0_FRE8	= 0x00000100,
	RIC0_FRE9	= 0x00000200,
	RIC0_FRE10	= 0x00000400,
	RIC0_FRE11	= 0x00000800,
	RIC0_FRE12	= 0x00001000,
	RIC0_FRE13	= 0x00002000,
	RIC0_FRE14	= 0x00004000,
	RIC0_FRE15	= 0x00008000,
	RIC0_FRE16	= 0x00010000,
	RIC0_FRE17	= 0x00020000,
};

enum RIS0_BIT {
	RIS0_FRF0	= 0x00000001,
	RIS0_FRF1	= 0x00000002,
	RIS0_FRF2	= 0x00000004,
	RIS0_FRF3	= 0x00000008,
	RIS0_FRF4	= 0x00000010,
	RIS0_FRF5	= 0x00000020,
	RIS0_FRF6	= 0x00000040,
	RIS0_FRF7	= 0x00000080,
	RIS0_FRF8	= 0x00000100,
	RIS0_FRF9	= 0x00000200,
	RIS0_FRF10	= 0x00000400,
	RIS0_FRF11	= 0x00000800,
	RIS0_FRF12	= 0x00001000,
	RIS0_FRF13	= 0x00002000,
	RIS0_FRF14	= 0x00004000,
	RIS0_FRF15	= 0x00008000,
	RIS0_FRF16	= 0x00010000,
	RIS0_FRF17	= 0x00020000,
};

enum RIC1_BIT {
	RIC1_RWE0	= 0x00000001,
	RIC1_RWE1	= 0x00000002,
	RIC1_RWE2	= 0x00000004,
	RIC1_RWE3	= 0x00000008,
	RIC1_RWE4	= 0x00000010,
	RIC1_RWE5	= 0x00000020,
	RIC1_RWE6	= 0x00000040,
	RIC1_RWE7	= 0x00000080,
	RIC1_RWE8	= 0x00000100,
	RIC1_RWE9	= 0x00000200,
	RIC1_RWE10	= 0x00000400,
	RIC1_RWE11	= 0x00000800,
	RIC1_RWE12	= 0x00001000,
	RIC1_RWE13	= 0x00002000,
	RIC1_RWE14	= 0x00004000,
	RIC1_RWE15	= 0x00008000,
	RIC1_RWE16	= 0x00010000,
	RIC1_RWE17	= 0x00020000,
	RIC1_RFWE	= 0x80000000,
};

enum RIS1_BIT {
	RIS1_RWF0	= 0x00000001,
	RIS1_RWF1	= 0x00000002,
	RIS1_RWF2	= 0x00000004,
	RIS1_RWF3	= 0x00000008,
	RIS1_RWF4	= 0x00000010,
	RIS1_RWF5	= 0x00000020,
	RIS1_RWF6	= 0x00000040,
	RIS1_RWF7	= 0x00000080,
	RIS1_RWF8	= 0x00000100,
	RIS1_RWF9	= 0x00000200,
	RIS1_RWF10	= 0x00000400,
	RIS1_RWF11	= 0x00000800,
	RIS1_RWF12	= 0x00001000,
	RIS1_RWF13	= 0x00002000,
	RIS1_RWF14	= 0x00004000,
	RIS1_RWF15	= 0x00008000,
	RIS1_RWF16	= 0x00010000,
	RIS1_RWF17	= 0x00020000,
	RIS1_RFWF	= 0x80000000,
};

enum RIC2_BIT {
	RIC2_QFE0	= 0x00000001,
	RIC2_QFE1	= 0x00000002,
	RIC2_QFE2	= 0x00000004,
	RIC2_QFE3	= 0x00000008,
	RIC2_QFE4	= 0x00000010,
	RIC2_QFE5	= 0x00000020,
	RIC2_QFE6	= 0x00000040,
	RIC2_QFE7	= 0x00000080,
	RIC2_QFE8	= 0x00000100,
	RIC2_QFE9	= 0x00000200,
	RIC2_QFE10	= 0x00000400,
	RIC2_QFE11	= 0x00000800,
	RIC2_QFE12	= 0x00001000,
	RIC2_QFE13	= 0x00002000,
	RIC2_QFE14	= 0x00004000,
	RIC2_QFE15	= 0x00008000,
	RIC2_QFE16	= 0x00010000,
	RIC2_QFE17	= 0x00020000,
	RIC2_RFFE	= 0x80000000,
};

enum RIS2_BIT {
	RIS2_QFF0	= 0x00000001,
	RIS2_QFF1	= 0x00000002,
	RIS2_QFF2	= 0x00000004,
	RIS2_QFF3	= 0x00000008,
	RIS2_QFF4	= 0x00000010,
	RIS2_QFF5	= 0x00000020,
	RIS2_QFF6	= 0x00000040,
	RIS2_QFF7	= 0x00000080,
	RIS2_QFF8	= 0x00000100,
	RIS2_QFF9	= 0x00000200,
	RIS2_QFF10	= 0x00000400,
	RIS2_QFF11	= 0x00000800,
	RIS2_QFF12	= 0x00001000,
	RIS2_QFF13	= 0x00002000,
	RIS2_QFF14	= 0x00004000,
	RIS2_QFF15	= 0x00008000,
	RIS2_QFF16	= 0x00010000,
	RIS2_QFF17	= 0x00020000,
	RIS2_RFFF	= 0x80000000,
};


enum TIC_BIT {
	TIC_FTE0	= 0x00000001,
	TIC_FTE1	= 0x00000002,
	TIC_FTE2	= 0x00000004,
	TIC_FTE3	= 0x00000008,
	TIC_TFUE	= 0x00000100,
	TIC_TFWE	= 0x00000200,
	TIC_MFUE	= 0x00000400,
	TIC_MFWE	= 0x00000800,
};

enum TIS_BIT {
	TIS_FTF0	= 0x00000001,
	TIS_FTF1	= 0x00000002,
	TIS_FTF2	= 0x00000004,
	TIS_FTF3	= 0x00000008,
	TIS_TFUF	= 0x00000100,
	TIS_TFWF	= 0x00000200,
	TIS_MFUF	= 0x00000400,
	TIS_MFWF	= 0x00000800,
};

enum GIC_BIT {
	GIC_PTCE	= 0x00000001,
	GIC_PTOE	= 0x00000002,
	GIC_PTME	= 0x00000004,
};

enum GIS_BIT {
	GIS_PTCF	= 0x00000001,
	GIS_PTOF	= 0x00000002,
	GIS_PTMF	= 0x00000004,
};

enum ISS_BIT {
	ISS_FRS	= 0x00000001,
	ISS_RWS	= 0x00000002,
	ISS_FTS	= 0x00000004,
	ISS_ES	= 0x00000040,
	ISS_MS	= 0x00000080,
	ISS_TFUS	= 0x00000100,
	ISS_TFWS	= 0x00000200,
	ISS_MFUS	= 0x00000400,
	ISS_MFWA	= 0x00000800,
	ISS_RFWS	= 0x00001000,
	ISS_CGIS	= 0x00002000,
	ISS_DPS1	= 0x00020000,
	ISS_DPS2	= 0x00040000,
	ISS_DPS3	= 0x00080000,
	ISS_DPS4	= 0x00100000,
	ISS_DPS5	= 0x00200000,
	ISS_DPS6	= 0x00400000,
	ISS_DPS7	= 0x00800000,
	ISS_DPS8	= 0x01000000,
	ISS_DPS9	= 0x02000000,
	ISS_DPS10	= 0x04000000,
	ISS_DPS11	= 0x08000000,
	ISS_DPS12	= 0x10000000,
	ISS_DPS13	= 0x20000000,
	ISS_DPS14	= 0x40000000,
	ISS_DPS15	= 0x80000000,
};

/* PIR */
enum PIR_BIT {
	PIR_MDI = 0x08, PIR_MDO = 0x04, PIR_MMD = 0x02, PIR_MDC = 0x01,
};

/* ECMR */
enum ECMR_BIT {
	ECMR_TRCCM = 0x04000000, ECMR_RCSC = 0x00800000, ECMR_DPAD = 0x00200000,
	ECMR_RZPF = 0x00100000, ECMR_PFR = 0x00040000, ECMR_RXF = 0x00020000,
	ECMR_MPDE = 0x00000200, ECMR_RE = 0x00000040, ECMR_TE = 0x00000020,
	ECMR_DM = 0x00000002, ECMR_PRM = 0x00000001,
};

#define ECMR_CHG_DM (ECMR_TRCCM | ECMR_RZPF | ECMR_PFR | ECMR_RXF)

/* MPR */
enum MPR_BIT {
	MPR_MP = 0x00000001,
};

static inline unsigned long ravb_reg_addr(struct ravb_dev *eth,
					    int enum_index)
{
	const u16 *reg_offset = ravb_offset_rcar_gen2;
	return BASE_IO_ADDR + reg_offset[enum_index];
}

static inline void ravb_write(struct ravb_dev *eth, unsigned long data,
				int enum_index)
{
	writel(data, ravb_reg_addr(eth, enum_index));
}

static inline unsigned long ravb_read(struct ravb_dev *eth,
					int enum_index)
{
	return readl(ravb_reg_addr(eth, enum_index));
}
