// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020, Renesas Electronics Corporation.
 */

#include <clk.h>
#include <dm.h>
#include <log.h>
#include <mailbox-uclass.h>
#include <malloc.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <fdtdec.h>
#include <linux/delay.h>

struct mbox_com {
	int				index;
	void __iomem	*addr_base;
};

struct mfis_mbox {
	struct mbox_com mbox_tx;
	struct mbox_com mbox_rx;
	u32 num_chans;
};

static int mfis_send_data(struct mbox_chan *chan, const void *data)
{
	struct mfis_mbox *mfis = dev_get_priv(chan->dev);

	writel(0x1, mfis->mbox_tx.addr_base);

	return 0;
}

static int mfis_recv(struct mbox_chan *chan, void *data)
{
	mdelay(1);
	dev_dbg(chan->dev, "chan=%p\n", chan);

	struct mfis_mbox *mfis = dev_get_priv(chan->dev);

	writel(0x0, mfis->mbox_tx.addr_base);

	return 0;
}

static int mfis_request(struct mbox_chan *chan)
{
	dev_dbg(chan->dev, "chan=%p\n", chan);

	return 0;
}

static int mfis_free(struct mbox_chan *chan)
{
	dev_dbg(chan->dev, "chan=%p\n", chan);

	return 0;
}

struct mbox_ops mfis_mbox_ops = {
	.request = mfis_request,
	.rfree = mfis_free,
	.send = mfis_send_data,
	.recv = mfis_recv,
};

static int mfis_mbox_probe(struct udevice *dev)
{
	struct mfis_mbox *mbox = dev_get_priv(dev);
	int count = 0;

	while (dev_read_addr_index(dev, count) != FDT_ADDR_T_NONE)
		count++;

	count--;

	mbox->num_chans = count;

	if (mbox->num_chans < 0)
		return -EINVAL;

	mbox->mbox_tx.addr_base = (void __iomem *)dev_read_addr_index(dev, 0);
	mbox->mbox_rx.addr_base = (void __iomem *)dev_read_addr_index(dev, 1);

	return 0;
}

static const struct udevice_id mfis_mbox_of_match[] = {
	{ .compatible = "renesas,mfis-mbox", },
	{},
};

U_BOOT_DRIVER(mfis_mbox) = {
	.name = "mfis_mbox",
	.id = UCLASS_MAILBOX,
	.of_match = mfis_mbox_of_match,
	.probe = mfis_mbox_probe,
	.priv_auto	= sizeof(struct mfis_mbox),
	.ops = &mfis_mbox_ops,
};

