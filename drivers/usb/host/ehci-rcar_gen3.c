/*
 * drivers/usb/host/ehci-rcar_gen3.
 * 	This file is EHCI HCD (Host Controller Driver) for USB.
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/ehci-rcar.h>
#include "ehci.h"

static u32 usb_base_address[] = {
	0xEE080000,	/* USB0 (EHCI) */
	0xEE0A0000,	/* USB1 (EHCI) */
	0xEE0C0000,	/* USB2 (EHCI) */
};

int ehci_hcd_stop(int index)
{
	int i;
	u32 base = usb_base_address[index];

	/* reset ehci */
	setbits_le32((uintptr_t)(base + EHCI_USBCMD), CMD_RESET);
	for (i = 100; i > 0; i--) {
		if (!(readl((uintptr_t)(base + EHCI_USBCMD)) & CMD_RESET))
			break;
		udelay(100);
	}

	if (!i)
		printf("error : ehci(%d) reset failed.\n", index);

	switch (index) {
	case 0:
		setbits_le32(SMSTPCR7, SMSTPCR703);
		break;
	case 1:
		setbits_le32(SMSTPCR7, SMSTPCR702);
		break;
	case 2:
		setbits_le32(SMSTPCR7, SMSTPCR701);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int ehci_hcd_init(int index, enum usb_init_type init,
	struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	u32 base;
	struct rmobile_ehci_reg *rehci;
	struct ahb_bridge *ahb;
	uint32_t cap_base;

	base = usb_base_address[index];
	switch (index) {
	case 0:
		clrbits_le32(SMSTPCR7, SMSTPCR703);
		break;
	case 1:
		clrbits_le32(SMSTPCR7, SMSTPCR702);
		break;
	case 2:
		clrbits_le32(SMSTPCR7, SMSTPCR701);
		break;
	default:
		return -EINVAL;
	}

	rehci = (struct rmobile_ehci_reg *)(uintptr_t)(base + EHCI_OFFSET);
	ahb = (struct ahb_bridge *)(uintptr_t)(base + AHB_OFFSET);

	/* underconstruction */

	/* Enable interrupt */
	setbits_le32(&ahb->int_enable, USBH_INTBEN | USBH_INTAEN);

	*hccr = (struct ehci_hccr *)((uintptr_t)&rehci->hciversion);
	cap_base = ehci_readl(&(*hccr)->cr_capbase);
	*hcor = (struct ehci_hcor *)((uintptr_t)*hccr + HC_LENGTH(cap_base));

	return 0;
}
