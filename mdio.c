/**
 * Copyright (c) 2025 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Based on https://github.com/cioban/arduino-projects/blob/master/smi/smi.ino
 */

#include "pico/stdlib.h"

#define PULSE_DELAY_US 10 // 50 kHz MDIO cycle. With 5 kHz the Linux mdio bus ran into a timeout.

const uint MDC_PIN = 14;
const uint MDIO_PIN = 15;

void mdio_init(void) {
    gpio_init(MDC_PIN);
    gpio_set_dir(MDC_PIN, GPIO_OUT);

    gpio_init(MDIO_PIN);
    gpio_set_dir(MDIO_PIN, GPIO_OUT);
}

void mdio_pulse(void) {
    // This function should be improved because the RP2040 is nice features that should be utilized

    gpio_put(MDC_PIN, false);
    busy_wait_us_32(PULSE_DELAY_US); // sleep_us creates CPU hangup - no idea why. Use busy wait instead

    gpio_put(MDC_PIN, true);
    busy_wait_us_32(PULSE_DELAY_US); // sleep_us creates CPU hangup - no idea why. Use busy wait instead
}

uint16_t mdio_read(uint8_t phy, uint8_t reg) {
    uint8_t byte;
    uint16_t word, data;

    /* MDIO pin is output */
    gpio_set_dir(MDIO_PIN, GPIO_OUT);

    gpio_put(MDIO_PIN, true);

    for (byte = 0; byte < 32; byte++)
        mdio_pulse();

    /* Stat code */
    gpio_put(MDIO_PIN, false);
    mdio_pulse();
    gpio_put(MDIO_PIN, true);
    mdio_pulse();

    /* Read OP Code */
    gpio_put(MDIO_PIN, true);
    mdio_pulse();
    gpio_put(MDIO_PIN, false);
    mdio_pulse();

    /* PHY address - 5 bits */
    for (byte = 0x10; byte != 0;)
    {
        if (byte & phy)
            gpio_put(MDIO_PIN, true);
        else
            gpio_put(MDIO_PIN, false);

        mdio_pulse();

        byte = byte >> 1;
    }
    /* REG address - 5 bits */
    for (byte = 0x10; byte != 0;)
    {
        if (byte & reg)
            gpio_put(MDIO_PIN, true);
        else
            gpio_put(MDIO_PIN, false);

        mdio_pulse();

        byte = byte >> 1;
    }
    /* Turn around bits */
    /* MDIO now is input */
    gpio_set_dir(MDIO_PIN, GPIO_IN);
    mdio_pulse();
    mdio_pulse();
    /* Data - 16 bits */
    data = 0;
    for (word = 0x8000; word != 0;)
    {

        if (gpio_get(MDIO_PIN))
            data |= word;

        mdio_pulse();

        word = word >> 1;
    }

    /* This is needed for some reason... */
    mdio_pulse();

    return data;
}

void mdio_write(uint8_t phy, uint8_t reg, uint16_t data)
{
    uint8_t byte;
    uint16_t word;

    /* MDIO pin is output */
    gpio_set_dir(MDIO_PIN, GPIO_OUT);

    gpio_put(MDIO_PIN, true);
    for (byte = 0; byte < 32; byte++)
        mdio_pulse();

    /* Stat code */
    gpio_put(MDIO_PIN, false);
    mdio_pulse();
    gpio_put(MDIO_PIN, true);
    mdio_pulse();

    /* Write OP Code */
    gpio_put(MDIO_PIN, false);
    mdio_pulse();
    gpio_put(MDIO_PIN, true);
    mdio_pulse();

    /* PHY address - 5 bits */
    for (byte = 0x10; byte != 0; byte = byte >> 1)
    {
        if (byte & phy)
            gpio_put(MDIO_PIN, true);
        else
            gpio_put(MDIO_PIN, false);
        mdio_pulse();
    }
    /* REG address - 5 bits */
    for (byte = 0x10; byte != 0; byte = byte >> 1)
    {
        if (byte & reg)
            gpio_put(MDIO_PIN, true);
        else
            gpio_put(MDIO_PIN, false);

        mdio_pulse();
    }
    /* Turn around bits */
    gpio_put(MDIO_PIN, true);
    mdio_pulse();
    gpio_put(MDIO_PIN, false);
    mdio_pulse();

    /* Data - 16 bits */
    for (word = 0x8000; word != 0; word = word >> 1)
    {
        if (word & data)
            gpio_put(MDIO_PIN, true);
        else
            gpio_put(MDIO_PIN, false);

        mdio_pulse();
    }

    /* This is needed for some reason... */
    mdio_pulse();
}