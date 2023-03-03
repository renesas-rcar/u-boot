// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/net/rswitch.c
 *     This file is driver for Renesas Ethernet RSwitch2 (Ethernet-TSN).
 *
 * Copyright (C) 2021  Renesas Electronics Corporation
 *
 * Based on the Renesas Ethernet AVB driver.
 */

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <miiphy.h>
#include <malloc.h>
#include <asm/cache.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <wait_bit.h>
#include <asm/io.h>
#include <asm/gpio.h>

#define RSWITCH_NUM_HW		5

#define ETHA_TO_GWCA(i)		((i) % 2)
#define GWCA_TO_HW_INDEX(i)	((i) + 3)
#define HW_INDEX_TO_GWCA(i)	((i) - 3)

#define RSWITCH_MAX_CTAG_PCP	7

/* Registers */
#define RSWITCH_COMA_OFFSET     0x00009000
#define RSWITCH_ETHA_OFFSET     0x0000a000      /* with RMAC */
#define RSWITCH_ETHA_SIZE       0x00002000      /* with RMAC */
#define RSWITCH_GWCA_OFFSET	0x00010000
#define RSWITCH_GWCA_SIZE	0x00002000

#define FWRO    0
#define CARO    RSWITCH_COMA_OFFSET
#define GWRO    0
#define TARO    0
#define RMRO    0x1000

enum rswitch_reg {
	EAMC		= TARO + 0x0000,
	EAMS		= TARO + 0x0004,
	EATDQDC		= TARO + 0x0060,
	EATTFC          = TARO + 0x0138,
	EATASRIRM	= TARO + 0x03E4,

	GWMC		= GWRO + 0x0000,
	GWMS		= GWRO + 0x0004,
	GWMTIRM		= GWRO + 0x0100,
	GWVCC		= GWRO + 0x0130,
	GWTTFC		= GWRO + 0x0138,
	GWDCBAC0	= GWRO + 0x0194,
	GWDCBAC1	= GWRO + 0x0198,
	GWTRC		= GWRO + 0x0200,
	GWARIRM		= GWRO + 0x0380,
	GWDCC		= GWRO + 0x0400,

	RRC		= CARO + 0x0004,
	RCEC		= CARO + 0x0008,
	RCDC		= CARO + 0x000C,
	CABPIRM		= CARO + 0x0140,

	FWPC0		= FWRO + 0x0100,
	FWPBFC		= FWRO + 0x4A00,
	FWPBFCSDC	= FWRO + 0x4A04,

	MPSM		= RMRO + 0x0000,
	MPIC		= RMRO + 0x0004,
	MRMAC0		= RMRO + 0x0084,
	MRMAC1		= RMRO + 0x0088,
	MRAFC		= RMRO + 0x008C,
	MRSCE		= RMRO + 0x0090,
	MRSCP		= RMRO + 0x0094,
	MLVC		= RMRO + 0x0180,
	MLBC		= RMRO + 0x0188,
	MXGMIIC		= RMRO + 0x0190,
	MPCH		= RMRO + 0x0194,
	MANM		= RMRO + 0x019C,
	MMIS0		= RMRO + 0x0210,
	MMIS1		= RMRO + 0x0220,
};

/* COMA */
#define RRC_RR		BIT(0)
#define RCEC_RCE	BIT(16)

#define CABPIRM_BPIOG	BIT(0)
#define CABPIRM_BPR	BIT(1)

/* MFWD */
#define FWPC0(i)		(FWPC0 + (i) * 0x10)
#define FWPC0_LTHTA     BIT(0)
#define FWPC0_IP4UE     BIT(3)
#define FWPC0_IP4TE     BIT(4)
#define FWPC0_IP4OE     BIT(5)
#define FWPC0_L2SE      BIT(9)
#define FWPC0_IP4EA     BIT(10)
#define FWPC0_IPDSA     BIT(12)
#define FWPC0_IPHLA     BIT(18)
#define FWPC0_MACSDA    BIT(20)
#define FWPC0_MACHLA    BIT(26)
#define FWPC0_MACHMA    BIT(27)
#define FWPC0_VLANSA    BIT(28)

#define FWPC0_DEFAULT   (FWPC0_LTHTA | FWPC0_IP4UE | FWPC0_IP4TE | \
			 FWPC0_IP4OE | FWPC0_L2SE | FWPC0_IP4EA | \
			 FWPC0_IPDSA | FWPC0_IPHLA | FWPC0_MACSDA | \
			 FWPC0_MACHLA | FWPC0_MACHMA | FWPC0_VLANSA)

#define FWPBFC(i)		(FWPBFC + (i) * 0x10)
#define FWPBFCSDC(j, i)		(FWPBFCSDC + (i) * 0x10 + (j) * 0x04)

/* ETHA */
#define EATASRIRM_TASRIOG	BIT(0)
#define EATASRIRM_TASRR		BIT(1)
#define EATDQDC(q)		(EATDQDC + (q) * 0x04)
#define EATDQDC_DQD		(0xff)

/* RMAC */
#define MPIC_PIS_GMII		0x02
#define MPIC_LSC_MASK		(0x07 << 3)
#define MPIC_LSC_100		(0x01 << 3)
#define MPIC_LSC_1000		(0x02 << 3)
#define MPIC_LSC_2500		(0x03 << 3)
#define MLVC_PLV		BIT(16)
#define MLVC_LVT		0x09
#define MMIS0_LVSS		0x02

#define MPIC_PSMCS_MASK		(0x7f << 16)
#define MPIC_PSMHT_MASK		(0x06 << 24)
#define MPIC_MDC_CLK_SET	(0x06050000)

#define MPSM_MFF_C45		BIT(2)
#define MPSM_MFF_C22		(0x0 << 2)
#define MPSM_PSME		BIT(0)

#define MDIO_READ_C45		0x03
#define MDIO_WRITE_C45		0x01
#define MDIO_ADDR_C45		0x00

#define MDIO_READ_C22           0x02
#define MDIO_WRITE_C22          0x01

#define MPSM_POP_MASK		(0x03 << 13)
#define MPSM_PRA_MASK		(0x1f << 8)
#define MPSM_PDA_MASK		(0x1f << 3)
#define MPSM_PRD_MASK		(0xffff << 16)

/* Completion flags */
#define MMIS1_PAACS		BIT(2) /* Address */
#define MMIS1_PWACS		BIT(1) /* Write */
#define MMIS1_PRACS		BIT(0) /* Read */
#define MMIS1_CLEAR_FLAGS	0xf

/* Serdes */
#define RSWITCH_SERDES_OFFSET			0x0400
#define RSWITCH_SERDES_BANK_SELECT		0x03fc
#define RSWITCH_SERDES_FUSE_OVERRIDE(n)		(0x2600 - (n) * 0x400)

#define BANK_180				0x0180
#define VR_XS_PMA_MP_12G_16G_25G_SRAM		0x026c
#define VR_XS_PMA_MP_12G_16G_25G_REF_CLK_CTRL	0x0244
#define VR_XS_PMA_MP_10G_MPLLA_CTRL2		0x01cc
#define VR_XS_PMA_MP_12G_16G_25G_MPLL_CMN_CTRL	0x01c0
#define VR_XS_PMA_MP_12G_16G_MPLLA_CTRL0	0x01c4
#define VR_XS_PMA_MP_12G_MPLLA_CTRL1		0x01c8
#define VR_XS_PMA_MP_12G_MPLLA_CTRL3		0x01dc
#define VR_XS_PMA_MP_12G_16G_25G_VCO_CAL_LD0	0x0248
#define VR_XS_PMA_MP_12G_VCO_CAL_REF0		0x0258
#define VR_XS_PMA_MP_12G_16G_25G_RX_GENCTRL1	0x0144
#define VR_XS_PMA_CONSUMER_10G_RX_GENCTRL4	0x01a0
#define VR_XS_PMA_MP_12G_16G_25G_TX_RATE_CTRL	0x00d0
#define VR_XS_PMA_MP_12G_16G_25G_RX_RATE_CTRL	0x0150
#define VR_XS_PMA_MP_12G_16G_TX_GENCTRL2	0x00c8
#define VR_XS_PMA_MP_12G_16G_RX_GENCTRL2	0x0148
#define VR_XS_PMA_MP_12G_AFE_DFE_EN_CTRL	0x0174
#define VR_XS_PMA_MP_12G_RX_EQ_CTRL0		0x0160
#define VR_XS_PMA_MP_10G_RX_IQ_CTRL0		0x01ac
#define VR_XS_PMA_MP_12G_16G_25G_TX_GENCTRL1	0x00c4
#define VR_XS_PMA_MP_12G_16G_TX_GENCTRL2	0x00c8
#define VR_XS_PMA_MP_12G_16G_RX_GENCTRL2	0x0148
#define VR_XS_PMA_MP_12G_16G_25G_TX_GENCTRL1	0x00c4
#define VR_XS_PMA_MP_12G_16G_25G_TX_EQ_CTRL0	0x00d8
#define VR_XS_PMA_MP_12G_16G_25G_TX_EQ_CTRL1	0x00dc
#define VR_XS_PMA_MP_12G_16G_MPLLB_CTRL0	0x01d0
#define VR_XS_PMA_MP_12G_MPLLB_CTRL1		0x01d4
#define VR_XS_PMA_MP_12G_16G_MPLLB_CTRL2	0x01d8
#define VR_XS_PMA_MP_12G_MPLLB_CTRL3		0x01e0

#define BANK_300				0x0300
#define SR_XS_PCS_CTRL1				0x0000
#define SR_XS_PCS_STS1				0x0004
#define SR_XS_PCS_CTRL2				0x001c

#define BANK_380				0x0380
#define VR_XS_PCS_DIG_CTRL1			0x0000
#define VR_XS_PCS_DEBUG_CTRL			0x0014
#define VR_XS_PCS_KR_CTRL			0x001c

#define BANK_1F00				0x1f00
#define SR_MII_CTRL				0x0000

#define BANK_1F80				0x1f80
#define VR_MII_AN_CTRL				0x0004

enum rswitch_serdes_mode {
	USXGMII,
	SGMII,
	COMBINATION,
};

/* ETHA */
enum rswitch_etha_mode {
	EAMC_OPC_RESET,
	EAMC_OPC_DISABLE,
	EAMC_OPC_CONFIG,
	EAMC_OPC_OPERATION,
};

#define EAMS_OPS_MASK	EAMC_OPC_OPERATION

/* GWCA */
enum rswitch_gwca_mode {
	GWMC_OPC_RESET,
	GWMC_OPC_DISABLE,
	GWMC_OPC_CONFIG,
	GWMC_OPC_OPERATION,
};

#define GWMS_OPS_MASK	GWMC_OPC_OPERATION

#define GWMTIRM_MTIOG		BIT(0)
#define GWMTIRM_MTR		BIT(1)
#define GWARIRM_ARIOG		BIT(0)
#define GWARIRM_ARR		BIT(1)
#define GWVCC_VEM_SC_TAG	(0x3 << 16)
#define GWDCBAC0_DCBAUP		(0xff)
#define GWTRC(i)		(GWTRC + (i) * 0x04)
#define GWDCC(i)		(GWDCC + (i) * 0x04)
#define	GWDCC_DQT		BIT(11)
#define GWDCC_BALR		BIT(24)

struct rswitch_etha {
	int			index;
	void __iomem		*addr;
	void __iomem		*serdes_common_addr;
	void __iomem		*serdes_addr;
	struct phy_device	*phydev;
	struct mii_dev		*bus;
	unsigned char		enetaddr[ARP_HLEN_ASCII + 1];
};

struct rswitch_gwca {
	int			index;
	void __iomem		*addr;
	int			num_chain;
};

/* Setting value */
#define LINK_SPEED_100		100
#define LINK_SPEED_1000		1000
#define LINK_SPEED_2500		2500

/* Decriptor */
#define RSWITCH_NUM_BASE_DESC		2
#define RSWITCH_TX_CHAIN_INDEX		1
#define RSWITCH_RX_CHAIN_INDEX		0
#define RSWITCH_NUM_TX_DESC		8
#define RSWITCH_NUM_RX_DESC		8

enum RX_DS_CC_BIT {
	RX_DS   = 0x0fff, /* Data size */
	RX_TR   = 0x1000, /* Truncation indication */
	RX_EI   = 0x2000, /* Error indication */
	RX_PS   = 0xc000, /* Padding selection */
};

enum DIE_DT {
	/* Frame data */
	DT_FSINGLE      = 0x80,
	DT_FSTART       = 0x90,
	DT_FMID         = 0xa0,
	DT_FEND         = 0xb8,

	/* Chain control */
	DT_LEMPTY       = 0xc0,
	DT_EEMPTY       = 0xd0,
	DT_LINKFIX      = 0x00,
	DT_LINK         = 0xe0,
	DT_EOS          = 0xf0,
	/* HW/SW arbitration */
	DT_FEMPTY       = 0x40,
	DT_FEMPTY_IS    = 0x10,
	DT_FEMPTY_IC    = 0x20,
	DT_FEMPTY_ND    = 0x38,
	DT_FEMPTY_START = 0x50,
	DT_FEMPTY_MID   = 0x60,
	DT_FEMPTY_END   = 0x70,

	DT_MASK         = 0xf0,
	DIE             = 0x08, /* Descriptor Interrupt Enable */
};

struct rswitch_desc {
	__le16 info_ds; /* Descriptor size */
	u8 die_dt;      /* Descriptor interrupt enable and type */
	__u8  dptrh;    /* Descriptor pointer MSB */
	__le32 dptrl;   /* Descriptor pointer LSW */
} __packed;

struct rswitch_rxdesc {
	struct rswitch_desc	data;
	struct rswitch_desc	link;
	u8			__pad[48];
	u8			packet[PKTSIZE_ALIGN];
} __packed;

struct rswitch_priv {
	void __iomem		*addr;
	struct rswitch_etha	etha;
	struct rswitch_gwca	gwca;
	struct rswitch_desc	bat_desc[RSWITCH_NUM_BASE_DESC];
	struct rswitch_desc	tx_desc[RSWITCH_NUM_TX_DESC];
	struct rswitch_rxdesc	rx_desc[RSWITCH_NUM_RX_DESC];
	u32			rx_desc_index;
	u32			tx_desc_index;

	struct clk		rsw_clk;
	struct clk		phy_clk;

	bool			parallel_mode;
};

static inline void rswitch_flush_dcache(u32 addr, u32 len)
{
	flush_dcache_range(addr, addr + len);
}

static inline void rswitch_invalidate_dcache(u32 addr, u32 len)
{
	u32 start = addr & ~((uintptr_t)ARCH_DMA_MINALIGN - 1);
	u32 end = roundup(addr + len, ARCH_DMA_MINALIGN);

	invalidate_dcache_range(start, end);
}

void rswitch_modify(struct rswitch_etha *etha, enum rswitch_reg reg, u32 clear, u32 set)
{
	writel((readl(etha->addr + reg) & ~clear) | set, etha->addr + reg);
}

#define RSWITCH_TIMEOUT_MS	1000
static int rswitch_reg_wait(void __iomem *addr, u32 offs, u32 mask, u32 expected)
{
	int i;

	for (i = 0; i < RSWITCH_TIMEOUT_MS; i++) {
		if ((readl(addr + offs) & mask) == expected)
			return 0;

		mdelay(1);
	}

	return -ETIMEDOUT;
}

void rswitch_serdes_write32(void __iomem *addr, u32 offs,  u32 bank, u32 data)
{
	writel(bank, addr + RSWITCH_SERDES_BANK_SELECT);
	writel(data, addr + offs);
}

u32 rswitch_serdes_read32(void __iomem *addr, u32 offs,  u32 bank)
{
	writel(bank, addr + RSWITCH_SERDES_BANK_SELECT);
	return readl(addr + offs);
}

static int rswitch_serdes_reg_wait(void __iomem *addr, u32 offs, u32 bank, u32 mask, u32 expected)
{
	int i;

	writel(bank, addr + RSWITCH_SERDES_BANK_SELECT);
	mdelay(1);

	for (i = 0; i < RSWITCH_TIMEOUT_MS; i++) {
		if ((readl(addr + offs) & mask) == expected)
			return 0;
		mdelay(1);
	}

	return -ETIMEDOUT;
}

static bool rswitch_agent_clock_is_enabled(struct rswitch_priv *priv, int port)
{
	u32 val = readl(priv->addr + RCEC);

	if (val & RCEC_RCE)
		return (val & BIT(port)) ? true : false;
	else
		return false;
}

static void rswitch_agent_clock_ctrl(struct rswitch_priv *priv, int port, int enable)
{
	u32 val;

	if (enable) {
		val = readl(priv->addr + RCEC);
		writel(val | RCEC_RCE | BIT(port), priv->addr + RCEC);
	} else {
		val = readl(priv->addr + RCDC);
		writel(val | BIT(port), priv->addr + RCDC);
	}
}

static int rswitch_serdes_common_initialize_sram(struct rswitch_etha *etha)
{
	int ret;

	ret = rswitch_serdes_reg_wait(etha->serdes_common_addr, VR_XS_PMA_MP_12G_16G_25G_SRAM,
				      BANK_180, BIT(0), 0x01);
	if (ret)
		return ret;

	rswitch_serdes_write32(etha->serdes_common_addr, VR_XS_PMA_MP_12G_16G_25G_SRAM,
			       BANK_180, 0x3);

	ret = rswitch_serdes_reg_wait(etha->serdes_common_addr, SR_XS_PCS_CTRL1, BANK_300,
				      BIT(15), 0);
	if (ret)
		return ret;

	return 0;
}

static int rswitch_serdes_common_setting(struct rswitch_etha *etha, enum rswitch_serdes_mode mode)
{
	void __iomem *addr = etha->serdes_common_addr;

	switch (mode) {
	case SGMII:
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_REF_CLK_CTRL, BANK_180, 0x97);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_MPLLB_CTRL0, BANK_180, 0x60);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_MPLLB_CTRL2, BANK_180, 0x2200);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_MPLLB_CTRL1, BANK_180, 0);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_MPLLB_CTRL3, BANK_180, 0x3d);

		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int rswitch_serdes_chan_setting(struct rswitch_etha *etha, enum rswitch_serdes_mode mode)
{
	void __iomem *addr = etha->serdes_addr;
	int ret;

	switch (mode) {
	case SGMII:
		rswitch_serdes_write32(addr, SR_XS_PCS_CTRL2, BANK_300, 0x01);
		rswitch_serdes_write32(addr, VR_XS_PCS_DIG_CTRL1, BANK_380, 0x2000);

		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_MPLL_CMN_CTRL, BANK_180, 0x11);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_VCO_CAL_LD0, BANK_180, 0x540);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_VCO_CAL_REF0, BANK_180, 0x15);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_RX_GENCTRL1, BANK_180, 0x100);
		rswitch_serdes_write32(addr, VR_XS_PMA_CONSUMER_10G_RX_GENCTRL4, BANK_180, 0);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_TX_RATE_CTRL, BANK_180, 0x02);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_RX_RATE_CTRL, BANK_180, 0x03);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_TX_GENCTRL2, BANK_180, 0x100);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_RX_GENCTRL2, BANK_180, 0x100);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_AFE_DFE_EN_CTRL, BANK_180, 0);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_RX_EQ_CTRL0, BANK_180, 0x07);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_10G_RX_IQ_CTRL0, BANK_180, 0);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_TX_GENCTRL1, BANK_180, 0x310);

		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_TX_GENCTRL2, BANK_180, 0x0101);
		ret = rswitch_serdes_reg_wait(addr, VR_XS_PMA_MP_12G_16G_TX_GENCTRL2, BANK_180, BIT(0), 0);
		if (ret)
			return ret;

		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_RX_GENCTRL2, BANK_180, 0x101);
		ret = rswitch_serdes_reg_wait(addr, VR_XS_PMA_MP_12G_16G_RX_GENCTRL2, BANK_180, BIT(0), 0);
		if (ret)
			return ret;

		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_TX_GENCTRL1, BANK_180, 0x1310);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_TX_EQ_CTRL0, BANK_180, 0x1800);
		rswitch_serdes_write32(addr, VR_XS_PMA_MP_12G_16G_25G_TX_EQ_CTRL1, BANK_180, 0);

		rswitch_serdes_write32(addr, VR_XS_PCS_DIG_CTRL1, BANK_380, 0x2100);
		ret = rswitch_serdes_reg_wait(addr, VR_XS_PCS_DIG_CTRL1, BANK_380, BIT(8), 0);
		if (ret)
			return ret;
		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int rswitch_serdes_set_speed(struct rswitch_etha *etha, enum rswitch_serdes_mode mode)
{
	void __iomem *addr = etha->serdes_addr;

	switch (mode) {
	case SGMII:
		if (etha->phydev->speed == 1000)
			rswitch_serdes_write32(addr, SR_MII_CTRL, BANK_1F00, 0x140);
		else if (etha->phydev->speed == 100)
			rswitch_serdes_write32(addr, SR_MII_CTRL, BANK_1F00, 0x2100);
		else if (etha->phydev->speed == 10)
			rswitch_serdes_write32(addr, SR_MII_CTRL, BANK_1F00, 0x100);

		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int rswitch_serdes_init(struct rswitch_etha *etha)
{
	int ret;
	enum rswitch_serdes_mode mode;

	/* TODO: Support more modes */

	switch (etha->phydev->interface) {
	case PHY_INTERFACE_MODE_SGMII:
		mode = SGMII;
		break;
	default:
		debug("%s: Don't support this interface", __func__);
		return -ENOTSUPP;
	}

	/* Initialize SRAM */
	ret = rswitch_serdes_common_initialize_sram(etha);
	if (ret)
		return ret;

	/* Disable FUSE_OVERRIDE_EN */
	if (readl(etha->serdes_addr + RSWITCH_SERDES_FUSE_OVERRIDE(etha->index)))
		writel(0, etha->serdes_addr + RSWITCH_SERDES_FUSE_OVERRIDE(etha->index));

	/* Set common settings*/
	ret = rswitch_serdes_common_setting(etha, mode);
	if (ret)
		return ret;

	/* Assert softreset for PHY */
	rswitch_serdes_write32(etha->serdes_common_addr, VR_XS_PCS_DIG_CTRL1, BANK_380, 0x8000);

	/* Initialize SRAM */
	ret = rswitch_serdes_common_initialize_sram(etha);
	if (ret)
		return ret;

	/* Set channel settings*/
	ret = rswitch_serdes_chan_setting(etha, mode);
	if (ret)
		return ret;

	/* Set speed (bps) */
	ret = rswitch_serdes_set_speed(etha, mode);
	if (ret)
		return ret;

	ret = rswitch_serdes_reg_wait(etha->serdes_addr, SR_XS_PCS_STS1, BANK_300, BIT(2), BIT(2));
	if (ret) {
		debug("\n%s: SerDes Link up failed", __func__);
		return ret;
	}

	return 0;
}

static int rswitch_etha_change_mode(struct rswitch_priv *priv,
				    enum rswitch_etha_mode mode)
{
	struct rswitch_etha *etha = &priv->etha;
	int ret;

	/* Enable clock */
	if (!rswitch_agent_clock_is_enabled(priv, etha->index))
		rswitch_agent_clock_ctrl(priv, etha->index, 1);

	writel(mode, etha->addr + EAMC);

	ret = rswitch_reg_wait(etha->addr, EAMS, EAMS_OPS_MASK, mode);

	/* Disable clock */
	if (mode == EAMC_OPC_DISABLE)
		rswitch_agent_clock_ctrl(priv, etha->index, 0);

	return ret;
}

static int rswitch_gwca_change_mode(struct rswitch_priv *priv,
				    enum rswitch_gwca_mode mode)
{
	struct rswitch_gwca *gwca = &priv->gwca;
	int ret;

	/* Enable clock */
	if (!rswitch_agent_clock_is_enabled(priv, gwca->index))
		rswitch_agent_clock_ctrl(priv, gwca->index, 1);

	writel(mode, gwca->addr + GWMC);

	ret = rswitch_reg_wait(gwca->addr, GWMS, GWMS_OPS_MASK, mode);

	/* Disable clock */
	if (mode == GWMC_OPC_DISABLE)
		rswitch_agent_clock_ctrl(priv, gwca->index, 0);

	return ret;
}

static int rswitch_mii_access_c45(struct rswitch_etha *etha, bool read,
				  int phyad, int devad, int regad, int data)
{
	int pop = read ? MDIO_READ_C45 : MDIO_WRITE_C45;
	u32 val;
	int ret;

	/* No match device */
	if (devad == 0xffffffff)
		return 0;

	/* Clear completion flags */
	writel(MMIS1_CLEAR_FLAGS, etha->addr + MMIS1);

	/* Submit address to PHY (MDIO_ADDR_C45 << 13) */
	val = MPSM_PSME | MPSM_MFF_C45;
	writel((regad << 16) | (devad << 8) | (phyad << 3) | val, etha->addr + MPSM);

	ret = rswitch_reg_wait(etha->addr, MMIS1, MMIS1_PAACS, MMIS1_PAACS);
	if (ret)
		return ret;

	/* Clear address completion flag */
	rswitch_modify(etha, MMIS1, MMIS1_PAACS, MMIS1_PAACS);

	/* Read/Write PHY register */
	if (read) {
		writel((pop << 13) | (devad << 8) | (phyad << 3) | val, etha->addr + MPSM);

		ret = rswitch_reg_wait(etha->addr, MMIS1, MMIS1_PRACS, MMIS1_PRACS);
		if (ret)
			return ret;

		/* Read data */
		ret = (readl(etha->addr + MPSM) & MPSM_PRD_MASK) >> 16;

		/* Clear read completion flag */
		rswitch_modify(etha, MMIS1, MMIS1_PRACS, MMIS1_PRACS);

	} else {
		writel((data << 16) | (pop << 13) | (devad << 8) | (phyad << 3) | val,
		       etha->addr + MPSM);

		ret = rswitch_reg_wait(etha->addr, MMIS1, MMIS1_PWACS, MMIS1_PWACS);
	}

	return ret;
}

static int rswitch_mii_read_c45(struct mii_dev *miidev, int phyad, int devad, int regad)
{
	struct rswitch_priv *priv = miidev->priv;
	struct rswitch_etha *etha = &priv->etha;
	int val;

	val = rswitch_mii_access_c45(etha, true, phyad, devad, regad, 0);

	return val;
}

int rswitch_mii_write_c45(struct mii_dev *miidev, int phyad, int devad, int regad, u16 data)
{
	struct rswitch_priv *priv = miidev->priv;
	struct rswitch_etha *etha = &priv->etha;

	rswitch_mii_access_c45(etha, false, phyad, devad, regad, data);

	return 0;
}

static int rswitch_check_link(struct rswitch_etha *etha)
{
	int ret;

	/* Request Link Verification */
	writel(MLVC_PLV, etha->addr + MLVC);

	/* Complete Link Verification */
	ret = rswitch_reg_wait(etha->addr, MLVC, MLVC_PLV, 0);

	if (ret) {
		debug("\n%s: Link verification timeout!", __func__);
		return ret;
	}

	return 0;
}

static int rswitch_reset(struct rswitch_priv *priv)
{
	int ret = 0;

	if (!priv->parallel_mode) {
		setbits_le32(priv->addr + RRC, RRC_RR);
		clrbits_le32(priv->addr + RRC, RRC_RR);

		ret = rswitch_gwca_change_mode(priv, GWMC_OPC_DISABLE);
		if (ret)
			return ret;

		ret = rswitch_etha_change_mode(priv, EAMC_OPC_DISABLE);
		if (ret)
			return ret;
	} else {
		/* Wait until the GWCA.GWMC used by G4MH/CR52 becomes OPERATION */
		int retry = 100;

		do {
			ret = rswitch_reg_wait(priv->addr, RSWITCH_GWCA_OFFSET + GWMS,
					       GWMS_OPS_MASK, GWMC_OPC_OPERATION);
			retry--;
		} while (ret && retry != 0);
	}

	return ret;
}

static void rswitch_bat_desc_init(struct rswitch_priv *priv)
{
	const u32 desc_size = RSWITCH_NUM_BASE_DESC * sizeof(struct rswitch_desc);
	int i;

	/* Initialize all descriptors */
	memset(priv->bat_desc, 0x0, desc_size);

	for (i = 0; i < RSWITCH_NUM_BASE_DESC; i++)
		priv->bat_desc[i].die_dt = DT_EOS;

	rswitch_flush_dcache((uintptr_t)priv->bat_desc, desc_size);
}

static void rswitch_tx_desc_init(struct rswitch_priv *priv)
{
	const u32 desc_size = RSWITCH_NUM_TX_DESC * sizeof(struct rswitch_desc);
	int i;
	u64 tx_desc_addr;

	/* Initialize all descriptor */
	memset(priv->tx_desc, 0x0, desc_size);
	priv->tx_desc_index = 0;

	for (i = 0; i < RSWITCH_NUM_TX_DESC; i++)
		priv->tx_desc[i].die_dt = DT_EEMPTY;

	/* Mark the end of the descriptors */
	priv->tx_desc[RSWITCH_NUM_TX_DESC - 1].die_dt = DT_LINKFIX;
	tx_desc_addr = (uintptr_t)priv->tx_desc;
	priv->tx_desc[RSWITCH_NUM_TX_DESC - 1].dptrl = lower_32_bits(tx_desc_addr);
	priv->tx_desc[RSWITCH_NUM_TX_DESC - 1].dptrh = upper_32_bits(tx_desc_addr);
	rswitch_flush_dcache(tx_desc_addr, desc_size);

	/* Point the controller to the TX descriptor list */
	priv->bat_desc[RSWITCH_TX_CHAIN_INDEX].die_dt = DT_LINKFIX;
	priv->bat_desc[RSWITCH_TX_CHAIN_INDEX].dptrl = lower_32_bits(tx_desc_addr);
	priv->bat_desc[RSWITCH_TX_CHAIN_INDEX].dptrh = upper_32_bits(tx_desc_addr);
	rswitch_flush_dcache((uintptr_t)&priv->bat_desc[RSWITCH_TX_CHAIN_INDEX],
			     sizeof(struct rswitch_desc));
}

static void rswitch_rx_desc_init(struct rswitch_priv *priv)
{
	const u32 desc_size = RSWITCH_NUM_RX_DESC * sizeof(struct rswitch_rxdesc);
	int i;
	u64 packet_addr;
	u64 next_rx_desc_addr;
	u64 rx_desc_addr;

	/* Initialize all descriptor */
	memset(priv->rx_desc, 0x0, desc_size);
	priv->rx_desc_index = 0;

	for (i = 0; i < RSWITCH_NUM_RX_DESC; i++) {
		priv->rx_desc[i].data.die_dt = DT_EEMPTY;
		priv->rx_desc[i].data.info_ds = PKTSIZE_ALIGN;
		packet_addr = (uintptr_t)priv->rx_desc[i].packet;
		priv->rx_desc[i].data.dptrl = lower_32_bits(packet_addr);
		priv->rx_desc[i].data.dptrh = upper_32_bits(packet_addr);

		priv->rx_desc[i].link.die_dt = DT_LINKFIX;
		next_rx_desc_addr = (uintptr_t)&priv->rx_desc[i + 1];
		priv->rx_desc[i].link.dptrl = lower_32_bits(next_rx_desc_addr);
		priv->rx_desc[i].link.dptrh = upper_32_bits(next_rx_desc_addr);
	}

	/* Mark the end of the descriptors */
	priv->rx_desc[RSWITCH_NUM_RX_DESC - 1].link.die_dt = DT_LINKFIX;
	rx_desc_addr = (uintptr_t)priv->rx_desc;
	priv->rx_desc[RSWITCH_NUM_RX_DESC - 1].link.dptrl = lower_32_bits(rx_desc_addr);
	priv->rx_desc[RSWITCH_NUM_RX_DESC - 1].link.dptrh = upper_32_bits(rx_desc_addr);
	rswitch_flush_dcache(rx_desc_addr, desc_size);

	/* Point the controller to the rx descriptor list */
	priv->bat_desc[RSWITCH_RX_CHAIN_INDEX].die_dt = DT_LINKFIX;
	priv->bat_desc[RSWITCH_RX_CHAIN_INDEX].dptrl = lower_32_bits(rx_desc_addr);
	priv->bat_desc[RSWITCH_RX_CHAIN_INDEX].dptrh = upper_32_bits(rx_desc_addr);
	rswitch_flush_dcache((uintptr_t)&priv->bat_desc[RSWITCH_RX_CHAIN_INDEX],
			     sizeof(struct rswitch_desc));
}

static void rswitch_clock_enable(struct rswitch_priv *priv)
{
	struct rswitch_etha *etha = &priv->etha;
	struct rswitch_gwca *gwca = &priv->gwca;

	writel(BIT(etha->index) | BIT(gwca->index) |
	       RCEC_RCE, priv->addr + RCEC);
}

static int rswitch_bpool_init(struct rswitch_priv *priv)
{
	writel(CABPIRM_BPIOG, priv->addr + CABPIRM);

	return rswitch_reg_wait(priv->addr, CABPIRM, CABPIRM_BPR, CABPIRM_BPR);
}

static void rswitch_mfwd_init(struct rswitch_priv *priv)
{
	struct rswitch_etha *etha = &priv->etha;
	struct rswitch_gwca *gwca = &priv->gwca;

	writel(FWPC0_DEFAULT, priv->addr + FWPC0(etha->index));
	writel(FWPC0_DEFAULT, priv->addr + FWPC0(gwca->index));

	writel(RSWITCH_RX_CHAIN_INDEX,
	       priv->addr + FWPBFCSDC(HW_INDEX_TO_GWCA(gwca->index), etha->index));

	writel(BIT(gwca->index),
	       priv->addr + FWPBFC(etha->index));

	writel(BIT(etha->index),
	       priv->addr + FWPBFC(gwca->index));
}

static void rswitch_rmac_init(struct rswitch_etha *etha)
{
	unsigned char *mac = &etha->enetaddr[0];
	u32 reg;

	/* Set MAC address */
	writel((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5],
	       etha->addr + MRMAC1);

	writel((mac[0] << 8) | mac[1], etha->addr + MRMAC0);

	/* Set MIIx */
	writel(MPIC_PIS_GMII | MPIC_LSC_1000, etha->addr + MPIC);

	writel(0x07E707E7, etha->addr + MRAFC);

	/* Enable Station Management clock */
	reg = readl(etha->addr + MPIC);
	reg &= ~MPIC_PSMCS_MASK & ~MPIC_PSMHT_MASK;
	writel(reg | MPIC_MDC_CLK_SET, etha->addr + MPIC);

	/* Set Station Management Mode : Clause 45 */
	rswitch_modify(etha, MPSM, MPSM_MFF_C45, MPSM_MFF_C45);
}

static int rswitch_gwca_mcast_table_reset(struct rswitch_gwca *gwca)
{
	writel(GWMTIRM_MTIOG, gwca->addr + GWMTIRM);

	return rswitch_reg_wait(gwca->addr, GWMTIRM, GWMTIRM_MTR, GWMTIRM_MTR);
}

static int rswitch_gwca_axi_ram_reset(struct rswitch_gwca *gwca)
{
	writel(GWARIRM_ARIOG, gwca->addr + GWARIRM);

	return rswitch_reg_wait(gwca->addr, GWARIRM, GWARIRM_ARR, GWARIRM_ARR);
}

static int rswitch_gwca_init(struct rswitch_priv *priv)
{
	struct rswitch_gwca *gwca = &priv->gwca;
	int ret;

	ret = rswitch_gwca_change_mode(priv, GWMC_OPC_DISABLE);
	if (ret)
		return ret;

	ret = rswitch_gwca_change_mode(priv, GWMC_OPC_CONFIG);
	if (ret)
		return ret;

	ret = rswitch_gwca_mcast_table_reset(gwca);
	if (ret)
		return ret;

	ret = rswitch_gwca_axi_ram_reset(gwca);
	if (ret)
		return ret;

	/* Setting flow*/
	writel(GWVCC_VEM_SC_TAG, gwca->addr + GWVCC);
	writel(0, gwca->addr + GWTTFC);
	writel(upper_32_bits((uintptr_t)priv->bat_desc) & GWDCBAC0_DCBAUP, gwca->addr + GWDCBAC0);
	writel(lower_32_bits((uintptr_t)priv->bat_desc), gwca->addr + GWDCBAC1);
	writel(GWDCC_DQT | GWDCC_BALR, gwca->addr + GWDCC(RSWITCH_TX_CHAIN_INDEX));
	writel(GWDCC_BALR, gwca->addr + GWDCC(RSWITCH_RX_CHAIN_INDEX));

	ret = rswitch_gwca_change_mode(priv, GWMC_OPC_DISABLE);
	if (ret)
		return ret;

	ret = rswitch_gwca_change_mode(priv, GWMC_OPC_OPERATION);
	if (ret)
		return ret;

	return 0;
}

static int rswitch_etha_tas_ram_reset(struct rswitch_etha *etha)
{
	writel(EATASRIRM_TASRIOG, etha->addr + EATASRIRM);

	return rswitch_reg_wait(etha->addr, EATASRIRM, EATASRIRM_TASRR, EATASRIRM_TASRR);
}

static int rswitch_etha_init(struct rswitch_priv *priv)
{
	struct rswitch_etha *etha = &priv->etha;
	int ret;
	u32 prio;

	ret = rswitch_etha_change_mode(priv, EAMC_OPC_DISABLE);
	if (ret)
		return ret;

	ret = rswitch_etha_change_mode(priv, EAMC_OPC_CONFIG);
	if (ret)
		return ret;

	ret = rswitch_etha_tas_ram_reset(etha);
	if (ret)
		return ret;

	/* Setting flow */
	writel(0, etha->addr + EATTFC);

	for (prio = 0; prio < RSWITCH_MAX_CTAG_PCP; prio++)
		writel(EATDQDC_DQD, etha->addr + EATDQDC(prio));

	rswitch_rmac_init(etha);

	ret = rswitch_etha_change_mode(priv, EAMC_OPC_OPERATION);
	if (ret)
		return ret;

	return 0;
}

static int rswitch_phy_config(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_etha *etha = &priv->etha;
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct ofnode_phandle_args phandle_args;
	struct phy_device *phydev;
	int phy_addr;

	if (dev_read_phandle_with_args(dev, "phy-handle", NULL, 0, 0, &phandle_args))
		phy_addr = -1;
	else
		phy_addr = ofnode_read_u32_default(phandle_args.node, "reg", 1);

	phydev = phy_connect(etha->bus, phy_addr, dev, pdata->phy_interface);
	if (!phydev)
		return -ENODEV;

	etha->phydev = phydev;
	phydev->speed = 1000;

	/* Add workaround to let phy_{read/write}_mmd() know
	 * it can be accessed via PHY clause 45 directly.
	 */
	phydev->drv->features = PHY_10G_FEATURES;

	phy_config(phydev);

	return 0;
}

static int rswitch_init(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	int ret;

	ret = rswitch_reset(priv);
	if (ret)
		return ret;

	rswitch_bat_desc_init(priv);
	rswitch_tx_desc_init(priv);
	rswitch_rx_desc_init(priv);

	if (!priv->parallel_mode) {
		rswitch_clock_enable(priv);

		ret = rswitch_bpool_init(priv);
		if (ret)
			return ret;

		rswitch_mfwd_init(priv);
	}

	ret = rswitch_gwca_init(priv);
	if (ret)
		return ret;

	if (!priv->parallel_mode) {
		ret = rswitch_etha_init(priv);
		if (ret)
			return ret;

		ret = rswitch_phy_config(dev);
		if (ret)
			return ret;

		ret = rswitch_serdes_init(&priv->etha);
		if (ret)
			return ret;
	}

	return 0;
}

static int rswitch_start(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);

	if (!priv->parallel_mode) {
		int ret;

		ret = phy_startup(priv->etha.phydev);
		if (ret)
			return ret;

		/* Link Verification */
		ret = rswitch_check_link(&priv->etha);
		if (ret)
			return ret;
	}

	return 0;
}

#define RSWITCH_TX_TIMEOUT_MS	1000
static int rswitch_send(struct udevice *dev, void *packet, int len)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_desc *desc = &priv->tx_desc[priv->tx_desc_index];
	struct rswitch_gwca *gwca = &priv->gwca;
	u32 gwtrc_index, val, start;

	/* Update TX descriptor */
	rswitch_flush_dcache((uintptr_t)packet, len);
	memset(desc, 0x0, sizeof(*desc));
	desc->die_dt = DT_FSINGLE;
	desc->info_ds = len;
	desc->dptrl = lower_32_bits((uintptr_t)packet);
	desc->dptrh = upper_32_bits((uintptr_t)packet);
	rswitch_flush_dcache((uintptr_t)desc, sizeof(*desc));

	/* Start tranmission */
	gwtrc_index = RSWITCH_TX_CHAIN_INDEX / 32;
	val = readl(gwca->addr + GWTRC(gwtrc_index));
	writel(val | BIT(RSWITCH_TX_CHAIN_INDEX), gwca->addr + GWTRC(gwtrc_index));

	/* Wait until packet is transmitted */
	start = get_timer(0);
	while (get_timer(start) < RSWITCH_TX_TIMEOUT_MS) {
		rswitch_invalidate_dcache((uintptr_t)desc, sizeof(*desc));
		if ((desc->die_dt & DT_MASK) != DT_FSINGLE)
			break;
		udelay(10);
	}

	if (get_timer(start) >= RSWITCH_TX_TIMEOUT_MS) {
		debug("\n%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	priv->tx_desc_index = (priv->tx_desc_index + 1) % (RSWITCH_NUM_TX_DESC - 1);

	return 0;
}

static int rswitch_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_rxdesc *desc = &priv->rx_desc[priv->rx_desc_index];
	int len;
	u8 *packet;

	/* Check if the rx descriptor is ready */
	rswitch_invalidate_dcache((uintptr_t)desc, sizeof(*desc));
	if ((desc->data.die_dt & DT_MASK) == DT_FEMPTY)
		return -EAGAIN;

	len = desc->data.info_ds & RX_DS;
	packet = (u8 *)(((uintptr_t)(desc->data.dptrh) << 32) | (uintptr_t)desc->data.dptrl);
	rswitch_invalidate_dcache((uintptr_t)packet, len);

	*packetp = packet;

	return len;
}

static int rswitch_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_rxdesc *desc = &priv->rx_desc[priv->rx_desc_index];

	/* Make current descritor available again */
	desc->data.die_dt = DT_FEMPTY;
	desc->data.info_ds = PKTSIZE_ALIGN;
	rswitch_flush_dcache((uintptr_t)desc, sizeof(*desc));

	/*Point to the next descriptor */
	priv->rx_desc_index = (priv->rx_desc_index + 1) % RSWITCH_NUM_RX_DESC;
	desc = &priv->rx_desc[priv->rx_desc_index];
	rswitch_invalidate_dcache((uintptr_t)desc, sizeof(*desc));

	return 0;
}

static void rswitch_stop(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);

	if (!priv->parallel_mode)
		phy_shutdown(priv->etha.phydev);
}

static int rswitch_read_rom_hwaddr(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_etha *etha = &priv->etha;
	struct eth_pdata *pdata = dev_get_platdata(dev);
	u32 maca = readl(etha->addr + MRMAC0);
	u32 macb = readl(etha->addr + MRMAC1);
	char buf[ARP_HLEN_ASCII + 1];

	if (!priv->parallel_mode)
		return -EOPNOTSUPP;

	pdata->enetaddr[0] = (maca >>  8) & 0xff;
	pdata->enetaddr[1] = (maca >>  0) & 0xff;
	pdata->enetaddr[2] = (macb >> 24) & 0xff;
	pdata->enetaddr[3] = (macb >> 16) & 0xff;
	pdata->enetaddr[4] = (macb >>  8) & 0xff;
	pdata->enetaddr[5] = (macb >>  0) & 0xff;

	/*
	 * In case parallel mode enabled, the value from ROM will be used
	 * regardless the one in environment variable.
	 */
	if (is_valid_ethaddr(pdata->enetaddr)) {
		sprintf(buf, "%pM", pdata->enetaddr);
		env_set("ethaddr", buf);
	}

	return !is_valid_ethaddr(pdata->enetaddr);
}

static int rswitch_write_hwaddr(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_etha *etha = &priv->etha;
	struct eth_pdata *pdata = dev_get_platdata(dev);
	unsigned char *mac = pdata->enetaddr;

	writel((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5], etha->addr + MRMAC1);

	writel((mac[0] << 8) | mac[1], etha->addr + MRMAC0);

	return 0;
}

static int rswitch_probe(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_etha *etha = &priv->etha;
	struct rswitch_gwca *gwca = &priv->gwca;
	struct mii_dev *mdiodev;
	fdt_addr_t serdes_addr;
	fdt_size_t size;
	int ret;
	char *s;

	s = env_get("rswitch.parallel_mode");
	priv->parallel_mode = ((int)simple_strtol(s, NULL, 10) == 1) ? true : false;

	pdata->iobase = dev_read_addr_size_name(dev, "iobase", &size);
	priv->addr = map_physmem(pdata->iobase, size, MAP_NOCACHE);

	etha->index = CONFIG_RENESAS_ETHER_SWITCH_DEFAULT_PORT;
	etha->addr = priv->addr + RSWITCH_ETHA_OFFSET + etha->index * RSWITCH_ETHA_SIZE;

	serdes_addr = dev_read_addr_size_name(dev, "serdes", &size);
	etha->serdes_common_addr = map_physmem(serdes_addr, size, MAP_NOCACHE);
	etha->serdes_addr = etha->serdes_common_addr + etha->index * RSWITCH_SERDES_OFFSET;

	/* Use only one port so always use (forward to) GWCA1 */
	gwca->index = 1;
	gwca->addr = priv->addr + RSWITCH_GWCA_OFFSET + gwca->index * RSWITCH_GWCA_SIZE;
	gwca->index = GWCA_TO_HW_INDEX(gwca->index);

	eth_env_get_enetaddr("ethaddr", &etha->enetaddr[0]);

	if (priv->parallel_mode)
		return 0;

	ret = clk_get_by_name(dev, "rsw2", &priv->rsw_clk) |
	      clk_get_by_name(dev, "eth-phy", &priv->phy_clk);
	if (ret)
		goto err_mdio_alloc;

	ret = dev_read_phandle(dev);
	if (!ret) {
		ret = -ENODEV;
		goto err_mdio_alloc;
	}

	mdiodev = mdio_alloc();
	if (!mdiodev) {
		ret = -ENOMEM;
		goto err_mdio_alloc;
	}

	mdiodev->priv = priv;
	mdiodev->read = rswitch_mii_read_c45;
	mdiodev->write = rswitch_mii_write_c45;
	snprintf(mdiodev->name, sizeof(mdiodev->name), dev->name);

	ret = mdio_register(mdiodev);
	if (ret)
		goto err_mdio_register;

	priv->etha.bus = miiphy_get_dev_by_name(dev->name);

	ret = clk_enable(&priv->rsw_clk) &
	      clk_enable(&priv->phy_clk);

	ret = rswitch_init(dev);
	if (ret)
		goto err_mdio_reset;

	return 0;

err_mdio_reset:
	clk_disable(&priv->rsw_clk);
	clk_disable(&priv->phy_clk);
err_mdio_register:
	mdio_free(mdiodev);
err_mdio_alloc:
	unmap_physmem(priv->addr, MAP_NOCACHE);
	unmap_physmem(etha->serdes_addr, MAP_NOCACHE);
	return ret;
}

static int rswitch_remove(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	int ret;

	/* Turn off GWCA to make sure that there will be no new packets */
	ret = rswitch_gwca_change_mode(priv, GWMC_OPC_DISABLE);
	if (ret)
		pr_err("Failed to disable GWCA: %d\n", ret);

	if (!priv->parallel_mode) {
		clk_disable(&priv->rsw_clk);
		clk_disable(&priv->phy_clk);

		free(priv->etha.phydev);
		mdio_unregister(priv->etha.bus);
	}

	unmap_physmem(priv->addr, MAP_NOCACHE);
	unmap_physmem(priv->etha.serdes_addr, MAP_NOCACHE);

	return 0;
}

int rswitch_ofdata_to_platdata(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	const char *phy_mode;
	const fdt32_t *cell;
	int ret = 0;

	pdata->phy_interface = -1;
	phy_mode = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "phy-mode", NULL);

	if (phy_mode)
		pdata->phy_interface = phy_get_interface_by_name(phy_mode);

	if (pdata->phy_interface == -1) {
		debug("\n%s: Invalid PHY interface '%s'", __func__, phy_mode);
		return -EINVAL;
	}

	pdata->max_speed = 1000;
	cell = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "max-speed", NULL);
	if (cell)
		pdata->max_speed = fdt32_to_cpu(*cell);

	return ret;
}

static const struct eth_ops rswitch_ops = {
	.start		= rswitch_start,
	.send		= rswitch_send,
	.recv		= rswitch_recv,
	.free_pkt	= rswitch_free_pkt,
	.stop		= rswitch_stop,
	.write_hwaddr	= rswitch_write_hwaddr,
	.read_rom_hwaddr = rswitch_read_rom_hwaddr,
};

static const struct udevice_id rswitch_ids[] = {
	{ .compatible = "renesas,etherswitch" },
	{ }
};

U_BOOT_DRIVER(eth_rswitch) = {
	.name		= "rswitch",
	.id		= UCLASS_ETH,
	.of_match	= rswitch_ids,
	.ofdata_to_platdata = rswitch_ofdata_to_platdata,
	.probe		= rswitch_probe,
	.remove		= rswitch_remove,
	.ops		= &rswitch_ops,
	.priv_auto_alloc_size = sizeof(struct rswitch_priv),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
	.flags		= DM_FLAG_ALLOC_PRIV_DMA | DM_FLAG_OS_PREPARE,
};
