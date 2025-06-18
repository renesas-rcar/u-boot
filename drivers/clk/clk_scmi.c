// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019-2022 Linaro Limited
 */

#define LOG_CATEGORY UCLASS_CLK

#include <clk-uclass.h>
#include <dm.h>
#include <scmi_agent.h>
#include <scmi_protocols.h>
#include <asm/types.h>
#include <linux/clk-provider.h>

#if defined(CONFIG_RCAR_GEN5) && defined(CONFIG_RCAR_SCP_FIXUP)
enum clk_cmd {
    DESCRIBE_RATES,
    ATTRIBUTES,
    RATE_GET,
    RATE_SET,
    CFG_GET_SET
};

static bool is_clkid_ng(int clkcmd, int clkid)
{
    switch (clkcmd) {
        case ATTRIBUTES:
            if ((0 <= clkid && 92 >= clkid) ||
                (105 <= clkid && 120 >= clkid) ||
                (133 <= clkid && 148 >= clkid) ||
                (161 <= clkid && 176 >= clkid) ||
                (189 <= clkid && 196 >= clkid) ||
                (335 == clkid) ||
                (340 <= clkid && 343 >= clkid) ||
                (350 == clkid) ||
                (532 <= clkid && 537 >= clkid) ||
                (553 <= clkid && 579 >= clkid) ||
                (688 <= clkid && 700 >= clkid) ||
                (753 <= clkid && 754 >= clkid))
                return true;
            break;

        case DESCRIBE_RATES:
            if ((790 <= clkid && 940 >= clkid))
                return true;
            break;

        case RATE_SET:
            if ((927 <= clkid && 930 >= clkid) ||
                (915 == clkid) || (917 == clkid) ||
                (897 == clkid) ||
                (923 == clkid) || (933 == clkid) ||
				(830 <= clkid && 831 >= clkid) ||
				(849 == clkid) ||
				(864 <= clkid && 865 >= clkid) ||
				(872 == clkid) ||
				(874 == clkid) ||
				(876 <= clkid && 878 >= clkid) ||
				(881 <= clkid && 882 >= clkid) ||
				(884 <= clkid && 887 >= clkid) ||
				(888 == clkid) ||
				(890 == clkid) ||
				(893 == clkid) || (896 == clkid) ||
				(898 == clkid) || (907 == clkid) ||
				(909 == clkid) || (911 == clkid) ||
				(913 == clkid) || (940 == clkid) ||
				(919 <= clkid && 921 >= clkid) ||
				(924 <= clkid && 925 >= clkid))
                return true;
            break;

        case RATE_GET:
            if ((0 <= clkid && 825 >= clkid) ||
                (844 <= clkid && 848 >= clkid) ||
                (873 == clkid) ||
                (879 <= clkid && 880 >= clkid) ||
                (883 == clkid) ||
                (889 == clkid) ||
                (894 <= clkid && 895 >= clkid) )
                return true;
            break;

        case CFG_GET_SET:
            if ((849 <= clkid && 872 >= clkid) ||
                (874 <= clkid && 876 >= clkid) ||
                (878 == clkid) ||
                (881 == clkid) ||
                (884 == clkid) ||
                (899 <= clkid && 909 >= clkid) ||
                (917 == clkid) ||
                (921 == clkid) ||
                (924 <= clkid && 925 >= clkid) ||
                (928 == clkid) ||
                (930 == clkid) ||
                (933 <= clkid && 936 >= clkid))
                return true;
            break;

        default:
            break;
    }

    return false;
}
#endif /* CONFIG_RCAR_GEN5 && CONFIG_RCAR_SCP_FIXUP */

static int scmi_clk_get_num_clock(struct udevice *dev, size_t *num_clocks)
{
	struct scmi_clk_protocol_attr_out out;
	struct scmi_msg msg = {
		.protocol_id = SCMI_PROTOCOL_ID_CLOCK,
		.message_id = SCMI_PROTOCOL_ATTRIBUTES,
		.out_msg = (u8 *)&out,
		.out_msg_sz = sizeof(out),
	};
	int ret;

	ret = devm_scmi_process_msg(dev, &msg);
	if (ret)
		return ret;

	*num_clocks = out.attributes & SCMI_CLK_PROTO_ATTR_COUNT_MASK;

	return 0;
}

static int scmi_clk_get_attibute(struct udevice *dev, int clkid, char **name)
{
	struct scmi_clk_attribute_in in = {
		.clock_id = clkid,
	};
	struct scmi_clk_attribute_out out;
	struct scmi_msg msg = {
		.protocol_id = SCMI_PROTOCOL_ID_CLOCK,
		.message_id = SCMI_CLOCK_ATTRIBUTES,
		.in_msg = (u8 *)&in,
		.in_msg_sz = sizeof(in),
		.out_msg = (u8 *)&out,
		.out_msg_sz = sizeof(out),
	};
	int ret;

	ret = devm_scmi_process_msg(dev, &msg);
	if (ret)
		return ret;

	*name = strdup(out.clock_name);

	return 0;
}

static int scmi_clk_gate(struct clk *clk, int enable)
{
	struct scmi_clk_state_in in = {
		.clock_id = clk->id,
		.attributes = enable,
	};
	struct scmi_clk_state_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_CONFIG_SET,
					  in, out);
	int ret;

	ret = devm_scmi_process_msg(clk->dev, &msg);
	if (ret)
		return ret;

	return scmi_to_linux_errno(out.status);
}

static int scmi_clk_enable(struct clk *clk)
{
#if defined(CONFIG_RCAR_GEN5) && defined(CONFIG_RCAR_SCP_FIXUP)
	if (is_clkid_ng(CFG_GET_SET, clk->id))
		return -1;
#endif /* CONFIG_RCAR_GEN5 && CONFIG_RCAR_SCP_FIXUP */

	return scmi_clk_gate(clk, 1);
}

static int scmi_clk_disable(struct clk *clk)
{
#if defined(CONFIG_RCAR_GEN5) && defined(CONFIG_RCAR_SCP_FIXUP)
	if (is_clkid_ng(CFG_GET_SET, clk->id))
		return -1;
#endif /* CONFIG_RCAR_GEN5 && CONFIG_RCAR_SCP_FIXUP */

	return scmi_clk_gate(clk, 0);
}

static ulong scmi_clk_get_rate(struct clk *clk)
{
	struct scmi_clk_rate_get_in in = {
		.clock_id = clk->id,
	};
	struct scmi_clk_rate_get_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_RATE_GET,
					  in, out);
	int ret;

#if defined(CONFIG_RCAR_GEN5) && defined(CONFIG_RCAR_SCP_FIXUP)
	if (is_clkid_ng(RATE_GET, clk->id))
		return 0;
#endif /* CONFIG_RCAR_GEN5 && CONFIG_RCAR_SCP_FIXUP */

	ret = devm_scmi_process_msg(clk->dev, &msg);
	if (ret < 0)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	return (ulong)(((u64)out.rate_msb << 32) | out.rate_lsb);
}

static ulong scmi_clk_set_rate(struct clk *clk, ulong rate)
{
	struct scmi_clk_rate_set_in in = {
		.clock_id = clk->id,
		.flags = SCMI_CLK_RATE_ROUND_CLOSEST,
		.rate_lsb = (u32)rate,
		.rate_msb = (u32)((u64)rate >> 32),
	};
	struct scmi_clk_rate_set_out out;
	struct scmi_msg msg = SCMI_MSG_IN(SCMI_PROTOCOL_ID_CLOCK,
					  SCMI_CLOCK_RATE_SET,
					  in, out);
	int ret;

#if defined(CONFIG_RCAR_GEN5) && defined(CONFIG_RCAR_SCP_FIXUP)
	if (is_clkid_ng(RATE_GET, clk->id))
		return 0;
#endif /* CONFIG_RCAR_GEN5 && CONFIG_RCAR_SCP_FIXUP */

	ret = devm_scmi_process_msg(clk->dev, &msg);
	if (ret < 0)
		return ret;

	ret = scmi_to_linux_errno(out.status);
	if (ret < 0)
		return ret;

	return scmi_clk_get_rate(clk);
}

static int scmi_clk_probe(struct udevice *dev)
{
	struct clk *clk;
	size_t num_clocks, i;
	int ret;

	ret = devm_scmi_of_get_channel(dev);
	if (ret)
		return ret;

	if (!CONFIG_IS_ENABLED(CLK_CCF))
		return 0;

	/* register CCF children: CLK UCLASS, no probed again */
	if (device_get_uclass_id(dev->parent) == UCLASS_CLK)
		return 0;

	ret = scmi_clk_get_num_clock(dev, &num_clocks);
	if (ret)
		return ret;

	for (i = 0; i < num_clocks; i++) {
		char *clock_name;

#if defined(CONFIG_RCAR_SCP_FIXUP)
		if (is_clkid_ng(ATTRIBUTES, i)) {
			continue;
		}
#endif /* defined(CONFIG_RCAR_SCP_FIXUP) */

		if (!scmi_clk_get_attibute(dev, i, &clock_name)) {
			clk = kzalloc(sizeof(*clk), GFP_KERNEL);
			if (!clk || !clock_name)
				ret = -ENOMEM;
			else
				ret = clk_register(clk, dev->driver->name,
						   clock_name, dev->name);

			if (ret) {
				free(clk);
				free(clock_name);
				return ret;
			}

			clk_dm(i, clk);
		}
	}

	return 0;
}

static const struct clk_ops scmi_clk_ops = {
	.enable = scmi_clk_enable,
	.disable = scmi_clk_disable,
	.get_rate = scmi_clk_get_rate,
	.set_rate = scmi_clk_set_rate,
};

U_BOOT_DRIVER(scmi_clock) = {
	.name = "scmi_clk",
	.id = UCLASS_CLK,
	.ops = &scmi_clk_ops,
	.probe = scmi_clk_probe,
};
