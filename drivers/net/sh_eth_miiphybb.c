/*
 * drivers/net/sh_eth_miiphybb.c
 *     This file is MII bit-bang driver for Renesas ethernet controller.
 *
 * Copyright (C) 2015  Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <miiphy.h>

#include "sh_eth_miiphybb.h"

struct bb_miiphy_bus bb_miiphy_buses[] = {
#ifdef CONFIG_SH_ETHER
	{
		.name		= "sh_eth",
		.init		= sh_eth_bb_init,
		.mdio_active	= sh_eth_bb_mdio_active,
		.mdio_tristate	= sh_eth_bb_mdio_tristate,
		.set_mdio	= sh_eth_bb_set_mdio,
		.get_mdio	= sh_eth_bb_get_mdio,
		.set_mdc	= sh_eth_bb_set_mdc,
		.delay		= sh_eth_bb_delay,
	},
#endif
#ifdef CONFIG_RAVB
	{
		.name		= "ravb",
		.init		= ravb_bb_init,
		.mdio_active	= ravb_bb_mdio_active,
		.mdio_tristate	= ravb_bb_mdio_tristate,
		.set_mdio	= ravb_bb_set_mdio,
		.get_mdio	= ravb_bb_get_mdio,
		.set_mdc	= ravb_bb_set_mdc,
		.delay		= ravb_bb_delay,
	},
#endif
};
int bb_miiphy_buses_num = ARRAY_SIZE(bb_miiphy_buses);
