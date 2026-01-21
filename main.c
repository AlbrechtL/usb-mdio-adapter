/**
 * Copyright (c) 2025 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

#include <stdio.h>
#include "pico/stdlib.h"

#include "usb_mvmdio.h"
#include "realtek-smi.h"
#include "mdio.h"

#define VERSION "0.0.2"

uint16_t usb_mdio_pull_request_callback(uint8_t dev, uint8_t reg) {
    uint32_t reg_val = 0;
    reg_val = mdio_read(dev, reg);
    //realtek_smi_read_reg(((reg << 8) & dev), &reg_val);

    printf("MDIO read - dev: %i reg: %i reg_val: 0x%x\n", dev, reg, reg_val);

    return (uint16_t) reg_val;
}

void usb_mdio_push_request_callback(uint8_t dev, uint8_t reg, uint16_t reg_val) {
    printf("MDIO write - dev: %i reg: %i reg_val: 0x%x\n", dev, reg, reg_val);
    //mdio_write(dev, reg, reg_val);
    realtek_smi_write_reg(((reg << 8) & dev), (uint32_t) reg_val, true);
}

int main(void) {
    stdio_init_all();
    printf("\n");
    printf("%s startup\n", get_usb_product_string());
    printf("Copyright (c) 2025 Albrecht Lohofener\n");
    printf("Version %s\n", VERSION);
    printf("\n");

    // printf("Test MDIO - Read RTL8305SC MAC address\n");
    // uint16_t reg_val = mdio_read(3, 16);
    // printf("reg_val1=[0x%x]\n", reg_val);

    // reg_val = mdio_read(3, 17);
    // printf("reg_val2=[0x%x]\n", reg_val);

    // reg_val = mdio_read(3, 18);
    // printf("reg_val3=[0x%x]\n", reg_val);

    usb_device_init(&usb_mdio_pull_request_callback, &usb_mdio_push_request_callback);
    
    // Wait until configured
    while (!get_usb_configured()) {
        tight_loop_contents();
    }

    usb_start();

    // Everything is interrupt driven so just loop here
    while (1) {
        tight_loop_contents();
    }
}
