/*
 * drivers/net/sh_eth_miiphyb.h
 *     This file is MII bit-bang driver for Renesas ethernet controller.
 *
 * Copyright (C) 2015  Renesas Electronics Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <miiphy.h>

#ifdef CONFIG_SH_ETHER
extern int sh_eth_bb_init(struct bb_miiphy_bus *bus);
extern int sh_eth_bb_mdio_active(struct bb_miiphy_bus *bus);
extern int sh_eth_bb_mdio_tristate(struct bb_miiphy_bus *bus);
extern int sh_eth_bb_set_mdio(struct bb_miiphy_bus *bus, int v);
extern int sh_eth_bb_get_mdio(struct bb_miiphy_bus *bus, int *v);
extern int sh_eth_bb_set_mdc(struct bb_miiphy_bus *bus, int v);
extern int sh_eth_bb_delay(struct bb_miiphy_bus *bus);
#endif
#ifdef CONFIG_RAVB
extern int ravb_bb_init(struct bb_miiphy_bus *bus);
extern int ravb_bb_mdio_active(struct bb_miiphy_bus *bus);
extern int ravb_bb_mdio_tristate(struct bb_miiphy_bus *bus);
extern int ravb_bb_set_mdio(struct bb_miiphy_bus *bus, int v);
extern int ravb_bb_get_mdio(struct bb_miiphy_bus *bus, int *v);
extern int ravb_bb_set_mdc(struct bb_miiphy_bus *bus, int v);
extern int ravb_bb_delay(struct bb_miiphy_bus *bus);
#endif
