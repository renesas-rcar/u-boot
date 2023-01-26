// SPDX-License-Identifier: GPL-2.0+
/*
 * Renesas RCar Gen3 CPG MSSR driver
 *
 * Copyright (C) 2017 Marek Vasut <marek.vasut@gmail.com>
 *
 * Based on the following driver from Linux kernel:
 * r8a7796 Clock Pulse Generator / Module Standby and Software Reset
 *
 * Copyright (C) 2016 Glider bvba
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <wait_bit.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/clk-provider.h>

#include <dt-bindings/clock/renesas-cpg-mssr.h>

#include "rcar-cpg-lib.h"

#define SDnSRCFC_SHIFT 2
#define STPnHCK	BIT(9 - SDnSRCFC_SHIFT)

static const struct clk_div_table cpg_sdh_div_table[] = {
	{ 0, 1 }, { 1, 2 }, { STPnHCK | 2, 4 }, { STPnHCK | 3, 8 },
	{ STPnHCK | 4, 16 }, { 0, 0 },
};

static const struct clk_div_table cpg_sd_div_table[] = {
	{ 0, 2 }, { 1, 4 }, { 0, 0 },
};

static const struct clk_div_table cpg_rpc_div_table[] = {
	{ 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 }, { 0, 0 },
};

static unsigned int _get_table_div(const struct clk_div_table *table, u32 value)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == value)
			return clkt->div;
	return 0;
}

static unsigned int _get_table_val(const struct clk_div_table *table, unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return clkt->val;
	return 0;
}

#define div_mask(width)	((1 << (width)) - 1)

u64 rcar_clk_get_rate64_div_table(unsigned int parent, u64 parent_rate,
				  void __iomem *reg, u8 shift, u8 width,
				  const struct clk_div_table *table, char *name)
{
	u32 value, div;
	u64 rate;

	value = readl(reg) >> shift;
	value &= div_mask(width);

	div = _get_table_div(table, value);
	if (!div)
		return -EINVAL;

	rate = parent_rate / div;
	debug("%s[%i] %s clk: parent=%i div=%u => rate=%llu\n",
	      __func__, __LINE__, name, parent, div, rate);

	return rate;
}

int rcar_clk_set_rate64_div_table(unsigned int parent, u64 parent_rate, ulong rate,
				  void __iomem *reg, u8 shift, u8 width,
				  const struct clk_div_table *table, char *name)
{
	u32 value = 0, reg_val = 0, div = 0;

	div = parent_rate / rate;
	value = _get_table_val(table, div);
	if (!value)
		return -EINVAL;

	reg_val = readl(reg);
	reg_val &= ~(div_mask(width) << shift);
	reg_val |= value << shift;
	writel(reg_val, reg);

	debug("%s[%i] %s clk: parent=%i div=%u rate=%lu => val=%u\n",
	      __func__, __LINE__, name, parent, div, rate, value);

	return 0;
}

u64 rcar_clk_get_rate64_sdh(unsigned int parent, u64 parent_rate, void __iomem *reg)
{
	return rcar_clk_get_rate64_div_table(parent, parent_rate, reg,
					     SDnSRCFC_SHIFT, 8,
					     cpg_sdh_div_table, "SDH");
}

u64 rcar_clk_get_rate64_sd(unsigned int parent, u64 parent_rate, void __iomem *reg)
{
	return rcar_clk_get_rate64_div_table(parent, parent_rate, reg,
					     0, 2, cpg_sd_div_table, "SD");
}

u64 rcar_clk_get_rate64_rpc(unsigned int parent, u64 parent_rate, void __iomem *reg)
{
	return rcar_clk_get_rate64_div_table(parent, parent_rate, reg,
					     0, 3, cpg_rpc_div_table, "RPC");
}

u64 rcar_clk_get_rate64_rpcd2(unsigned int parent, u64 parent_rate)
{
	u64 rate = 0;

	rate = parent_rate / 2;
	debug("%s[%i] RPCD2 clk: parent=%i => rate=%llu\n",
	      __func__, __LINE__, parent, rate);

	return rate;
}

int rcar_clk_set_rate64_sdh(unsigned int parent, u64 parent_rate, ulong rate,
			    void __iomem *reg)
{
	return rcar_clk_set_rate64_div_table(parent, parent_rate, rate, reg,
					     SDnSRCFC_SHIFT, 8,
					     cpg_sdh_div_table, "SDH");
}

int rcar_clk_set_rate64_sd(unsigned int parent, u64 parent_rate, ulong rate,
			   void __iomem *reg)
{
	return rcar_clk_set_rate64_div_table(parent, parent_rate, rate, reg,
					     0, 2, cpg_sd_div_table, "SD");
}
