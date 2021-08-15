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

	FWPBFC		= FWRO + 0x4A00,

	MPSM		= RMRO + 0x0000,
	MPIC		= RMRO + 0x0004,
	MTFFC		= RMRO + 0x0020,
	MTPFC		= RMRO + 0x0024,
	MTPFC2		= RMRO + 0x0028,
	MTPFC3		= RMRO + 0x0030,
	MTATC		= RMRO + 0x0050,
	MRGC		= RMRO + 0x0080,
	MRMAC0		= RMRO + 0x0084,
	MRMAC1		= RMRO + 0x0088,
	MRAFC		= RMRO + 0x008C,
	MRSCE		= RMRO + 0x0090,
	MRSCP		= RMRO + 0x0094,
	MRFSCE		= RMRO + 0x009C,
	MRFSCP		= RMRO + 0x00A0,
	MTRC		= RMRO + 0x00A4,
	MLVC		= RMRO + 0x0180,
	MXGMIIC		= RMRO + 0x0190,
	MPCH		= RMRO + 0x0194,
	MANM		= RMRO + 0x019C,
	MEIE		= RMRO + 0x0204,
	MMIS0		= RMRO + 0x0210,
	MMIE0		= RMRO + 0x0214,
	MMIS1		= RMRO + 0x0220,
	MMIE1		= RMRO + 0x0224,
	MMIE2		= RMRO + 0x0234,
};

#define MTPFC3(x)	(MTPFC3 + 4*(x))
#define MTATC(x)	(MTATC + 4*(x))

/* COMA */
#define RRC_RR		BIT(0)
#define RCEC_RCE	BIT(16)

#define CABPIRM_BPIOG	BIT(0)
#define CABPIRM_BPR	BIT(1)

/* MFWD */
#define FWPBFC(i)	(FWPBFC + (i) * 0x10)

/* ETHA */
#define EATASRIRM_TASRIOG	BIT(0)
#define EATASRIRM_TASRR		BIT(1)
#define EATDQDC(q)		(EATDQDC + (q) * 0x04)
#define EATDQDC_DQD		(0xff)

/* RMAC */
#define MPIC_PIS_XGMII		0x100
#define MPIC_LSC_MASK		(0x7 << 3)
#define MPIC_LSC_100		0x01
#define MPIC_LSC_1000		0x10
#define MPIC_LSC_2500		0x11
#define MTFFC_FCM_PFC		0x2
#define MTFFC_FCM_NO_PAD	0x1
#define MTPFC3_PFC		0x1
#define MLVC_PLV_RQ		0x1000
#define MLVC_LVT		0x9
#define MMIS0_LVSS		0x2

#define MPIC_PSMCS_MASK		(0x7f << 16)
#define MPIC_PSMCS_SET		(0x40 << 16)
#define MPIC_PSMHT_MASK		(0x7 << 24)
#define MPIC_PSMHT_SET		(0x6 << 24)

#define MPSM_MFF_C45		(0x1 << 2)
#define MPSM_MFF_C22		(0x0 << 2)
#define MPSM_PSME_MASK		0x1
#define MPSM_MFF_MASK		0x4

#define MDIO_READ		0x11
#define MDIO_WRITE		0x01
#define MDIO_ADDR		0x00

#define MPSM_POP_MASK		(0x3 << 13)
#define MPSM_PRA_MASK		(0x1f << 8)
#define MPSM_PDA_MASK		(0x1f << 3)
#define MPSM_PRD_MASK		(0xff << 16)

/* Completion flags */
#define MMIS1_PAACS		BIT(2) /* Address */
#define MMIS1_PWACS		BIT(1) /* Write */
#define MMIS1_PRACS		BIT(0) /* Read */

#define LINK_TIMEOUT		1000

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
	struct phy_device	*phydev;
	struct mii_dev		*bus;
	unsigned char		*enetaddr;
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
#define RSWITCH_TX_CHAIN_INDEX		0
#define RSWITCH_RX_CHAIN_INDEX		1
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
	DT_FMID         = 0xA0,
	DT_FEND         = 0xB8,

	/* Chain control */
	DT_LEMPTY       = 0xC0,
	DT_EEMPTY       = 0xD0,
	DT_LINKFIX      = 0x00,
	DT_LINK         = 0xE0,
	DT_EOS          = 0xF0,
	/* HW/SW arbitration */
	DT_FEMPTY       = 0x40,
	DT_FEMPTY_IS    = 0x10,
	DT_FEMPTY_IC    = 0x20,
	DT_FEMPTY_ND    = 0x38,
	DT_FEMPTY_START = 0x50,
	DT_FEMPTY_MID   = 0x60,
	DT_FEMPTY_END   = 0x70,

	DT_MASK         = 0xF0,
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
	writel((readl(etha->addr + reg) & ~clear) | set, reg);
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
	int ret = 0;

	/* Set register address to PHY */
	rswitch_modify(etha, MPSM, MPSM_POP_MASK, MDIO_ADDR);
	rswitch_modify(etha, MPSM, MPSM_PRD_MASK, regad << 16);
	rswitch_modify(etha, MPSM, MPSM_PRA_MASK, devad << 8);
	rswitch_modify(etha, MPSM, MPSM_PDA_MASK, phyad << 3);

	/* Start PHY access */
	rswitch_modify(etha, MPSM, MPSM_PSME_MASK, 0x1);

	/* Wait to finish address setting*/
	rswitch_reg_wait(etha->addr, MPSM, MPSM_PSME_MASK, 0);

	/* Clear completion flag */
	rswitch_modify(etha, MMIS1, MMIS1_PAACS, 0);

	/* Read/Write PHY register */
	rswitch_modify(etha, MPSM, MPSM_POP_MASK, read ? MDIO_READ : MDIO_WRITE );
	rswitch_modify(etha, MPSM, MPSM_PDA_MASK, devad << 8);
	rswitch_modify(etha, MPSM, MPSM_PRA_MASK, phyad << 3);

	/* Start PHY access */
	rswitch_modify(etha, MPSM, MPSM_PSME_MASK, 0x1);

	if (!read)
		rswitch_modify(etha, MPSM, MPSM_PRD_MASK, data << 16);

	rswitch_reg_wait(etha->addr, MPSM, MPSM_PSME_MASK, 0);

	if (read)
		ret = (readl(etha->addr + MPSM) & MPSM_PRD_MASK) >> 16 ;

	return ret;
}

static int rswitch_mii_read_c45(struct mii_dev *miidev, int phyad, int devad, int regad)
{
	struct rswitch_priv *priv = miidev->priv;
	struct rswitch_etha *etha = &priv->etha;
	int val;

	/* Change to config mode */
	rswitch_etha_change_mode(priv, EAMC_OPC_CONFIG);

	/* Enable Station Management clock */
	rswitch_modify(etha, MPIC, MPIC_PSMHT_MASK, MPIC_PSMHT_SET);
	rswitch_modify(etha, MPIC, MPIC_PSMCS_MASK, MPIC_PSMCS_SET);

	/* Set Station Management Mode : Clause 45 */
	rswitch_modify(etha, MPSM, 0x4, MPSM_MFF_C45);

	/* Access PHY register */
	val = rswitch_mii_access_c45(etha, true, phyad, devad, regad, 0);

	/* Disale Station Management Clock */
	rswitch_modify(etha, MPIC, MPIC_PSMCS_MASK, 0x0);

	/* Change to disable mode */
	rswitch_etha_change_mode(priv, EAMC_OPC_DISABLE);

	return val;
}

int rswitch_mii_write_c45(struct mii_dev *miidev, int phyad, int devad, int regad, u16 data)
{
	struct rswitch_priv *priv = miidev->priv;
	struct rswitch_etha *etha = &priv->etha;

	/* Change to config mode */
	rswitch_etha_change_mode(priv, EAMC_OPC_CONFIG);

	/* Enable Station Management clock */
	rswitch_modify(etha, MPIC, MPIC_PSMHT_MASK, MPIC_PSMHT_SET);
	rswitch_modify(etha, MPIC, MPIC_PSMCS_MASK, MPIC_PSMCS_SET);

	/* Set Station Management Mode : Clause 45 */
	rswitch_modify(etha, MPSM, 0x4, MPSM_MFF_C45);

	/* Access PHY register */
	rswitch_mii_access_c45(etha, false, phyad, devad, regad, data);

	/* Disale Station Management Clock */
	rswitch_modify(etha, MPIC, MPIC_PSMCS_MASK, 0x0);

	/* Change to disable mode */
	rswitch_etha_change_mode(priv, EAMC_OPC_DISABLE);

	return 0;
}

static int rswitch_check_link(struct rswitch_etha *etha)
{
	int timeout = 0;
	int ret;
	/* Request Link Verification */
	writel(MLVC_PLV_RQ, etha->addr + MLVC);

	/* Complete Link Verification */
	while (readl(etha->addr + MLVC) & MLVC_PLV_RQ) {
		timeout++;

		if (timeout > LINK_TIMEOUT){
			debug("%s: Link verification timeout \n", __func__);
			ret = -ETIMEDOUT;
			break;
		}
	}
	/* Clear interrupt */
	writel(MMIS0_LVSS, etha->addr + MMIS0);

	return 0;
}

static int rswitch_reset(struct rswitch_priv *priv)
{
	int ret;

	ret = rswitch_gwca_change_mode(priv, GWMC_OPC_DISABLE);
	if (ret)
		return ret;

	ret = rswitch_etha_change_mode(priv, EAMC_OPC_DISABLE);
	if (ret)
		return ret;

	writel(RRC_RR, priv->addr + RRC);

	return 0;
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
		priv->rx_desc[i].link.dptrh = lower_32_bits(next_rx_desc_addr);
	}

	/* Mark the end of the descriptors */
	priv->rx_desc[RSWITCH_NUM_RX_DESC - 1].link.die_dt = DT_LINKFIX;
	rx_desc_addr = (uintptr_t)priv->rx_desc;
	priv->rx_desc[RSWITCH_NUM_RX_DESC - 1].link.dptrl = lower_32_bits(rx_desc_addr);
	priv->rx_desc[RSWITCH_NUM_RX_DESC - 1].link.dptrh = upper_32_bits(rx_desc_addr);

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

	writel(BIT(gwca->index),
	       priv->addr + FWPBFC(etha->index));

	writel(BIT(etha->index),
	       priv->addr + FWPBFC(gwca->index));
}

static int rswitch_rmac_init(struct rswitch_etha *etha)
{
	printf("\nRSW_DBG: %s", __func__);
	unsigned char *mac = etha->enetaddr;
	int ret;

	/* Set MAC address */
	writel((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5],
	       etha->addr + MRMAC1);

	writel((mac[0] << 8) | mac[1], etha->addr + MRMAC0);

	/* Set MIIx */
	writel(MPIC_PIS_XGMII | MPIC_LSC_1000, etha->addr + MPIC);

	/* Disable all MAC error interrupt */
	writel(0x0, etha->addr + MEIE);

	/* Disable MAC monitoring interrupt */
	writel(0x0, etha->addr + MMIE0);
	writel(0x0, etha->addr + MMIE1);
	writel(0x0, etha->addr + MMIE2);

	/* MAC transmission configuration */
	writel(MTFFC_FCM_PFC, etha->addr + MTFFC);
	writel(MTPFC3_PFC, etha->addr + MTPFC3(0));

	/* Link Verification */
	ret = rswitch_check_link(etha);
	if (ret)
		return ret;

	return 0;
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
	writel(GWDCC_BALR ,gwca->addr + GWDCC(RSWITCH_TX_CHAIN_INDEX));
	writel(GWDCC_DQT | GWDCC_BALR, gwca->addr + GWDCC(RSWITCH_RX_CHAIN_INDEX));

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

static int phy_update_speed(struct rswitch_etha *etha, int speed)
{
	/* Update speed */
	switch (speed) {
		case 100:
			/* Set MIIx */
			rswitch_modify(etha, MPIC, MPIC_LSC_MASK, MPIC_LSC_100);
			break;
		case 1000:
			rswitch_modify(etha, MPIC, MPIC_LSC_MASK, MPIC_LSC_1000);
			break;
		case 2500:
			rswitch_modify(etha, MPIC, MPIC_LSC_MASK, MPIC_LSC_2500);
			break;
		default:
			pr_info("%s : Not support speed %d", __func__, speed);
			break;
	}
	return 0;
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
	for (prio = 0; prio < RSWITCH_MAX_CTAG_PCP; prio++)
		writel(EATDQDC_DQD, etha->addr + EATDQDC(prio));

	ret = rswitch_rmac_init(etha);
	if (ret)
		return ret;

	ret = phy_startup(etha->phydev);
	if (ret)
		return ret;

	phy_update_speed(etha, etha->phydev->speed);

	ret = rswitch_etha_change_mode(priv, EAMC_OPC_OPERATION);
	if (ret)
		return ret;

	return 0;
}

static int rswitch_init(struct rswitch_priv *priv)
{
	int ret;

	ret = rswitch_reset(priv);
	if (ret)
		return ret;

	rswitch_bat_desc_init(priv);
	rswitch_tx_desc_init(priv);
	rswitch_rx_desc_init(priv);

	rswitch_clock_enable(priv);

	ret = rswitch_bpool_init(priv);
	if (ret)
		return ret;

	rswitch_mfwd_init(priv);

	ret = rswitch_gwca_init(priv);
	if (ret)
		return ret;

	ret = rswitch_etha_init(priv);
	if (ret)
		return ret;

	return 0;
}

static int rswitch_phy_config(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_etha *etha = &priv->etha;
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct phy_device *phydev;
	int reg;

	phydev = phy_connect(etha->bus, 0, dev, pdata->phy_interface);
	if (!phydev)
		return -ENODEV;

	etha->phydev = phydev;

	phydev->supported &= SUPPORTED_100baseT_Full | SUPPORTED_1000baseT_Full |
			     SUPPORTED_Autoneg |  SUPPORTED_TP | SUPPORTED_MII |
			     SUPPORTED_Pause | SUPPORTED_Asym_Pause;

	if (pdata->max_speed != 1000) {
		phydev->supported &= ~SUPPORTED_1000baseT_Full;
		reg = phy_read(phydev, -1, MII_CTRL1000);
		reg &= ~(BIT(9) | BIT(8));
		phy_write(phydev, -1, MII_CTRL1000, reg);
	}
	phydev->speed = 1000;
	phy_config(phydev);

	return 0;
}

static int rswitch_start(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	int ret;

	ret = rswitch_init(priv);
	if (ret)
		return ret;

	return 0;
}

#define RSWITCH_TX_TIMEOUT_MS	1000
static int rswitch_send(struct udevice *dev, void *packet, int len)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_desc *desc = &priv->tx_desc[priv->tx_desc_index];
	struct rswitch_gwca *gwca = &priv->gwca;
	u32 gwtrc_index, val, start;

	printf("\nRSW_DBG %s", __func__);

	/* Update TX descriptor */
	rswitch_flush_dcache((uintptr_t)packet, len);
	memset(desc, 0x0, sizeof(*desc));
	desc->die_dt = DT_FSINGLE;
	desc->dptrl = lower_32_bits((uintptr_t)packet);
	desc->dptrh = upper_32_bits((uintptr_t)packet);
	rswitch_flush_dcache((uintptr_t)desc, sizeof(*desc));

	/* Start tranmission */
	gwtrc_index = RSWITCH_TX_CHAIN_INDEX % 32;
	val = readl(gwca->addr + GWTRC(gwtrc_index));
	writel(val | BIT(RSWITCH_TX_CHAIN_INDEX), gwca->addr + GWTRC(gwtrc_index));

	/* Wait until packet is transmitted */
	start = get_timer(0);
	while (get_timer(start) < RSWITCH_TX_TIMEOUT_MS) {
		rswitch_invalidate_dcache((uintptr_t)desc, sizeof(*desc));
		if (desc->die_dt != DT_FSINGLE)
			break;
		udelay(10);
	}

	if (get_timer(start) >= RSWITCH_TX_TIMEOUT_MS)
		return -ETIMEDOUT;

	priv->tx_desc_index = (priv->tx_desc_index + 1) % (RSWITCH_NUM_TX_DESC - 1);

	return 0;
}

static int rswitch_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_rxdesc *desc = &priv->rx_desc[priv->rx_desc_index];
	struct rswitch_gwca *gwca = &priv->gwca;
	u32 val, gwtrc_index;
	int len;
	u8 *packet;

	printf("\nRSW_DBG %s", __func__);

	/* Check if the rx descriptor is ready */
	rswitch_invalidate_dcache((uintptr_t)desc, sizeof(*desc));
	if (desc->data.die_dt == DT_FEMPTY)
		return -EAGAIN;

	/* Start reception */
	gwtrc_index = RSWITCH_RX_CHAIN_INDEX % 32;
	val = readl(gwca->addr + GWTRC(gwtrc_index));
	writel(val | BIT(RSWITCH_RX_CHAIN_INDEX), gwca->addr + GWTRC(gwtrc_index));

	/* TODO: Check for error */


	len = desc->data.info_ds & RX_DS;
	packet = (u8*)(uintptr_t)desc->data.dptrl;
	rswitch_invalidate_dcache((uintptr_t)packet, len);

	*packetp = packet;

	return len;
}

static int rswitch_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_rxdesc *desc = &priv->rx_desc[priv->rx_desc_index];

	/* Make current descritor available again */
	desc->data.die_dt = DT_EEMPTY;
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

	phy_shutdown(priv->etha.phydev);
	rswitch_reset(priv);
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
	struct ofnode_phandle_args phandle_args;
	struct mii_dev *mdiodev;
	int ret;

	priv->addr = map_physmem(pdata->iobase, 0x20000, MAP_NOCACHE);
	/*
	 * TODO: Use only one port so parse from U-Boot env
	 *       then assign index for etha
	 * Currently, Fixed to use port 0
	 */
	etha->index = 0;
	etha->addr = priv->addr + RSWITCH_ETHA_OFFSET + etha->index * RSWITCH_ETHA_SIZE;

	etha->enetaddr = pdata->enetaddr;

	/* Use only one port so always use (forward to) GWCA0 */
	gwca->index = 0;
	gwca->addr = priv->addr + RSWITCH_GWCA_OFFSET + gwca->index * RSWITCH_GWCA_SIZE;
	gwca->index = GWCA_TO_HW_INDEX(gwca->index);

	ret = clk_get_by_name(dev, "rsw2", &priv->rsw_clk) |
	      clk_get_by_name(dev, "eth-phy", &priv->phy_clk);
	if (ret)
		goto err_mdio_alloc;

	ret = dev_read_phandle_with_args(dev, "phy-handle", NULL, 0, 0, &phandle_args);
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

	ret = rswitch_reset(priv);
	if (ret)
		goto err_mdio_reset;

	ret = rswitch_phy_config(dev);
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
	return ret;
}

static int rswitch_remove(struct udevice *dev)
{
	struct rswitch_priv *priv = dev_get_priv(dev);

	clk_disable(&priv->rsw_clk);
	clk_disable(&priv->phy_clk);

	free(priv->etha.phydev);
	mdio_unregister(priv->etha.bus);
	unmap_physmem(priv->addr, MAP_NOCACHE);

	return 0;
}

int rswitch_ofdata_to_platdata(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	const char *phy_mode;
	const fdt32_t *cell;
	int ret = 0;

	pdata->iobase = dev_read_addr(dev);
	pdata->phy_interface = -1;
	phy_mode = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "phy-mode",
				NULL);
	if (phy_mode)
		pdata->phy_interface = phy_get_interface_by_name(phy_mode);

	if (pdata->phy_interface == -1) {
		debug("%s: Invalid PHY interface '%s'\n", __func__, phy_mode);
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
