/*
 * ULCB board CPLD access support
 *
 * Copyright (C) 2017 Renesas Electronics Corporation
 * Copyright (C) 2017 Cogent Embedded, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <asm/arch/prr_depend.h>
#include <asm/arch/rcar-mstp.h>
#include <asm/io.h>
#include <asm/gpio.h>

#if defined(CONFIG_R8A7795)
#define SCLK	(rcar_is_legacy() ? ES_GPIO_GP_6_8 : GPIO_GP_6_8)
#define SSTBZ	(rcar_is_legacy() ? ES_GPIO_GP_2_3 : GPIO_GP_2_3)
#define MOSI	(rcar_is_legacy() ? ES_GPIO_GP_6_7 : GPIO_GP_6_7)
#define MISO	(rcar_is_legacy() ? ES_GPIO_GP_6_10 : GPIO_GP_6_10)
#elif defined(CONFIG_R8A7796X)
#define SCLK	GPIO_GP_6_8
#define SSTBZ	GPIO_GP_2_3
#define MOSI	GPIO_GP_6_7
#define MISO	GPIO_GP_6_10
#endif

#define GP2_MSTP910   (1 << 10)
#define GP6_MSTP906   (1 << 6)

#define CPLD_ADDR_MODE		0x00 /* RW */
#define CPLD_ADDR_MUX		0x02 /* RW */
#define CPLD_ADDR_DIPSW6	0x08 /* R */
#define CPLD_ADDR_RESET		0x80 /* RW */
#define CPLD_ADDR_VERSION	0xFF /* R */

/* LSI pin pull-up control */
#define PUEN_SSI_SDATA4		(1 << 17)

static int cpld_initialized;

static u32 cpld_read(u8 addr)
{
	int i;
	u32 data = 0;

	for (i = 0; i < 8; i++) {
		gpio_set_value(MOSI, addr & 0x80); /* MSB first */
		gpio_set_value(SCLK, 1);
		addr <<= 1;
		gpio_set_value(SCLK, 0);
	}

	gpio_set_value(MOSI, 0); /* READ */
	gpio_set_value(SSTBZ, 0);
	gpio_set_value(SCLK, 1);
	gpio_set_value(SCLK, 0);
	gpio_set_value(SSTBZ, 1);

	for (i = 0; i < 32; i++) {
		gpio_set_value(SCLK, 1);
		data <<= 1;
		data |= gpio_get_value(MISO); /* MSB first */
		gpio_set_value(SCLK, 0);
	}

	return data;
}

static void cpld_write(u8 addr, u32 data)
{
	int i;

	for (i = 0; i < 32; i++) {
		gpio_set_value(MOSI, data & (1 << 31)); /* MSB first */
		gpio_set_value(SCLK, 1);
		data <<= 1;
		gpio_set_value(SCLK, 0);
	}

	for (i = 0; i < 8; i++) {
		gpio_set_value(MOSI, addr & 0x80); /* MSB first */
		gpio_set_value(SCLK, 1);
		addr <<= 1;
		gpio_set_value(SCLK, 0);
	}

	gpio_set_value(MOSI, 1); /* WRITE */
	gpio_set_value(SSTBZ, 0);
	gpio_set_value(SCLK, 1);
	gpio_set_value(SCLK, 0);
	gpio_set_value(SSTBZ, 1);
}

static void cpld_init(void)
{
	u32 val;

	if (cpld_initialized)
		return;

	cpld_initialized = 1;

	/* PULL-UP on MISO line */
	val = readl(PFC_PUEN5);
	val |= PUEN_SSI_SDATA4;
	writel(val, PFC_PUEN5);

	/* GPIO2, GPIO6 for reset */
	mstp_clrbits_le32(SMSTPCR9, SMSTPCR9, GP6_MSTP906 | GP2_MSTP910);

	gpio_request(SCLK, NULL);
	gpio_request(SSTBZ, NULL);
	gpio_request(MOSI, NULL);
	gpio_request(MISO, NULL);

	gpio_direction_output(SCLK, 0);
	gpio_direction_output(SSTBZ, 1);
	gpio_direction_output(MOSI, 0);
	gpio_direction_input(MISO);

	/* dummy read */
	cpld_read(CPLD_ADDR_VERSION);
}

static int do_cpld(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 addr, val;

	cpld_init();

	if (argc == 2 && strcmp(argv[1], "info") == 0) {
		printf("CPLD version:              0x%08x\n",
		       cpld_read(CPLD_ADDR_VERSION));
		printf("H3 Mode setting (MD0..28): 0x%08x\n",
		       cpld_read(CPLD_ADDR_MODE));
		printf("Multiplexer settings:      0x%08x\n",
		       cpld_read(CPLD_ADDR_MUX));
		printf("DIPSW (SW6):               0x%08x\n",
		       cpld_read(CPLD_ADDR_DIPSW6));
		return 0;
	}

	if (argc < 3)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[2], NULL, 16);
	if (!(addr == CPLD_ADDR_VERSION || addr == CPLD_ADDR_MODE ||
	      addr == CPLD_ADDR_MUX || addr == CPLD_ADDR_DIPSW6 ||
	      addr == CPLD_ADDR_RESET)) {
		printf("cpld invalid addr\n");
		return CMD_RET_USAGE;
	}

	if (argc == 3 && strcmp(argv[1], "read") == 0) {
		printf("0x%x\n", cpld_read(addr));
	} else if (argc == 4 && strcmp(argv[1], "write") == 0) {
		val = simple_strtoul(argv[3], NULL, 16);
		cpld_write(addr, val);
	}

	return 0;
}

U_BOOT_CMD(
	cpld, 4, 1, do_cpld,
	"CPLD access",
	"info\n"
	"cpld read addr\n"
	"cpld write addr val\n"
);

void reset_cpu(ulong addr)
{
	cpld_init();
	cpld_write(CPLD_ADDR_RESET, 1);
}
