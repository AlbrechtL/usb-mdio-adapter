// SPDX-License-Identifier: GPL-2.0+
/* Realtek Simple Management Interface (SMI) driver
 * It can be discussed how "simple" this interface is.
 *
 * The SMI protocol piggy-backs the MDIO MDC and MDIO signals levels
 * but the protocol is not MDIO at all. Instead it is a Realtek
 * pecularity that need to bit-bang the lines in a special way to
 * communicate with the switch.
 *
 * ASICs we intend to support with this driver:
 *
 * RTL8366   - The original version, apparently
 * RTL8369   - Similar enough to have the same datsheet as RTL8366
 * RTL8366RB - Probably reads out "RTL8366 revision B", has a quite
 *             different register layout from the other two
 * RTL8366S  - Is this "RTL8366 super"?
 * RTL8367   - Has an OpenWRT driver as well
 * RTL8368S  - Seems to be an alternative name for RTL8366RB
 * RTL8370   - Also uses SMI
 * RTL8305   - 5 port switch
 *
 * Copyright (C) 2025 Albrecht Lohofener <albrechtloh@gmx.de>
 * Copyright (C) 2017 Linus Walleij <linus.walleij@linaro.org>
 * Copyright (C) 2010 Antti Seppälä <a.seppala@gmail.com>
 * Copyright (C) 2010 Roman Yeryomin <roman@advem.lv>
 * Copyright (C) 2011 Colin Leitner <colin.leitner@googlemail.com>
 * Copyright (C) 2009-2010 Gabor Juhos <juhosg@openwrt.org>
 */

#include <stdio.h>
#include <errno.h>

#include "pico/stdlib.h"

#include "realtek-smi.h"

#define REALTEK_SMI_ACK_RETRY_COUNT		5
#define PULSE_DELAY_US 20 // 50 kHz MDIO cycle. With 5 kHz the Linux mdio bus ran into a timeout.
#define MDC_PIN 14
#define MDIO_PIN 15

static inline void realtek_smi_clk_delay()
{
	busy_wait_us_32(PULSE_DELAY_US);
}

static void realtek_smi_start()
{
	gpio_init(MDC_PIN);
	gpio_init(MDIO_PIN);

	/* Set GPIO pins to output mode, with initial state:
	 * SCK = 0, SDA = 1
	 */
	gpio_set_dir(MDC_PIN, GPIO_OUT); gpio_put(MDC_PIN, 0);
	gpio_set_dir(MDIO_PIN, GPIO_OUT); gpio_put(MDIO_PIN, 1);
	realtek_smi_clk_delay();

	/* CLK 1: 0 -> 1, 1 -> 0 */
	gpio_set_dir(MDC_PIN, 1);
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 0);
	realtek_smi_clk_delay();

	/* CLK 2: */
	gpio_set_dir(MDC_PIN, 1);
	realtek_smi_clk_delay();
	gpio_set_dir(MDIO_PIN, 0);
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 0);
	realtek_smi_clk_delay();
	gpio_set_dir(MDIO_PIN, 1);
}

static void realtek_smi_stop()
{
	realtek_smi_clk_delay();
	gpio_set_dir(MDIO_PIN, 0);
	gpio_set_dir(MDC_PIN, 1);
	realtek_smi_clk_delay();
	gpio_set_dir(MDIO_PIN, 1);
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 1);
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 0);
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 1);

	/* Add a click */
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 0);
	realtek_smi_clk_delay();
	gpio_set_dir(MDC_PIN, 1);

	/* Set GPIO pins to input mode */
	gpio_set_dir(MDIO_PIN, GPIO_IN);
	gpio_set_dir(MDC_PIN, GPIO_IN);
}

static void realtek_smi_write_bits(uint32_t data, uint32_t len)
{
	for (; len > 0; len--) {
		realtek_smi_clk_delay();

		/* Prepare data */
		gpio_set_dir(MDIO_PIN, !!(data & (1 << (len - 1))));
		realtek_smi_clk_delay();

		/* Clocking */
		gpio_set_dir(MDC_PIN, 1);
		realtek_smi_clk_delay();
		gpio_set_dir(MDC_PIN, 0);
	}
}

static void realtek_smi_read_bits(uint32_t len, uint32_t *data)
{
	gpio_set_dir(MDIO_PIN, GPIO_IN);

	for (*data = 0; len > 0; len--) {
		uint32_t u;

		realtek_smi_clk_delay();

		/* Clocking */
		gpio_set_dir(MDC_PIN, 1);
		realtek_smi_clk_delay();
		u = !!gpio_get(MDIO_PIN);
		gpio_set_dir(MDC_PIN, 0);

		*data |= (u << (len - 1));
	}

	gpio_set_dir(MDIO_PIN, GPIO_OUT); gpio_put(MDIO_PIN, 0);
}

static int realtek_smi_wait_for_ack()
{
	int retry_cnt;

	retry_cnt = 0;
	do {
		uint32_t ack;

		realtek_smi_read_bits(1, &ack);
		if (ack == 0)
			break;

		if (++retry_cnt > REALTEK_SMI_ACK_RETRY_COUNT) {
			printf("ACK timeout\n");
			return -ETIMEDOUT;
		}
	} while (1);

	return 0;
}

static int realtek_smi_write_byte(uint8_t data)
{
	realtek_smi_write_bits(data, 8);
	return realtek_smi_wait_for_ack();
}

static int realtek_smi_write_byte_noack(uint8_t data)
{
	realtek_smi_write_bits(data, 8);
	return 0;
}

static int realtek_smi_read_byte0(uint8_t *data)
{
	uint32_t t;

	/* Read data */
	realtek_smi_read_bits(8, &t);
	*data = (t & 0xff);

	/* Send an ACK */
	realtek_smi_write_bits(0x00, 1);

	return 0;
}

static int realtek_smi_read_byte1(uint8_t *data)
{
	uint32_t t;

	/* Read data */
	realtek_smi_read_bits(8, &t);
	*data = (t & 0xff);

	/* Send an ACK */
	realtek_smi_write_bits(0x01, 1);

	return 0;
}

int realtek_smi_read_reg(uint32_t addr, uint32_t *data)
{
	unsigned long flags;
	uint8_t lo = 0;
	uint8_t hi = 0;
	int ret;

	realtek_smi_start();

	/* Send READ command */
	ret = realtek_smi_write_byte(0xa9);
	if (ret)
		goto out;

	/* Set ADDR[7:0] */
	ret = realtek_smi_write_byte(addr & 0xff);
	if (ret)
		goto out;

	/* Set ADDR[15:8] */
	ret = realtek_smi_write_byte(addr >> 8);
	if (ret)
		goto out;

	/* Read DATA[7:0] */
	realtek_smi_read_byte0(&lo);
	/* Read DATA[15:8] */
	realtek_smi_read_byte1(&hi);

	*data = ((uint32_t)lo) | (((uint32_t)hi) << 8);

	ret = 0;

 out:
	realtek_smi_stop();

	return ret;
}

int realtek_smi_write_reg(uint32_t addr, uint32_t data, bool ack)
{
	unsigned long flags;
	int ret;

	realtek_smi_start();

	/* Send WRITE command */
	ret = realtek_smi_write_byte(0xb8);
	if (ret)
		goto out;

	/* Set ADDR[7:0] */
	ret = realtek_smi_write_byte(addr & 0xff);
	if (ret)
		goto out;

	/* Set ADDR[15:8] */
	ret = realtek_smi_write_byte(addr >> 8);
	if (ret)
		goto out;

	/* Write DATA[7:0] */
	ret = realtek_smi_write_byte(data & 0xff);
	if (ret)
		goto out;

	/* Write DATA[15:8] */
	if (ack)
		ret = realtek_smi_write_byte(data >> 8);
	else
		ret = realtek_smi_write_byte_noack(data >> 8);
	if (ret)
		goto out;

	ret = 0;

 out:
	realtek_smi_stop();

	return ret;
}

/* There is one single case when we need to use this accessor and that
 * is when issueing soft reset. Since the device reset as soon as we write
 * that bit, no ACK will come back for natural reasons.
 */
static int realtek_smi_write_reg_noack(uint32_t reg, uint32_t val)
{
	return realtek_smi_write_reg(reg, val, false);
}

/* Regmap accessors */

static int realtek_smi_write(uint32_t reg, uint32_t val)
{
	return realtek_smi_write_reg(reg, val, true);
}

static int realtek_smi_read(uint32_t reg, uint32_t *val)
{
	return realtek_smi_read_reg(reg, val);
}