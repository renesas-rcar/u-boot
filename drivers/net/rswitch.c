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
};

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
};

struct rswitch_gwca {
	int			index;
	void __iomem		*addr;
	int			num_chain;
};

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

static int rswitch_rmac_init(struct rswitch_etha *etha) {

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
	return 0;
}

static int rswitch_probe(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct rswitch_priv *priv = dev_get_priv(dev);
	struct rswitch_etha *etha = &priv->etha;
	struct rswitch_gwca *gwca = &priv->gwca;
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

	/* Use only one port so always use (forward to) GWCA0 */
	gwca->index = 0;
	gwca->addr = priv->addr + RSWITCH_GWCA_OFFSET + gwca->index * RSWITCH_GWCA_SIZE;
	gwca->index = GWCA_TO_HW_INDEX(gwca->index);

	ret = clk_get_by_name(dev, "rsw2", &priv->rsw_clk) |
	      clk_get_by_name(dev, "eth-phy", &priv->phy_clk);
	if (ret)
		goto err_mdio_alloc;

	mdiodev = mdio_alloc();
	if (!mdiodev) {
		ret = -ENOMEM;
		goto err_mdio_alloc;
	}

	mdiodev->read = bb_miiphy_read;
	mdiodev->write = bb_miiphy_write;
	bb_miiphy_buses[0].priv = priv;
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

	pdata->iobase = dev_read_addr(dev);

	return 0;
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
