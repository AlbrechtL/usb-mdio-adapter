/**
 * Copyright (c) 2025 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

void usb_device_init(
    uint16_t (*_usb_mdio_pull_request_callback)(uint8_t, uint8_t),
    void (*_usb_mdio_push_request_callback)(uint8_t, uint8_t, uint16_t));
void usb_start(void);

bool get_usb_configured(void);
unsigned char * get_usb_product_string(void);