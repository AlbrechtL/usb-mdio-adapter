/**
 * Copyright (c) 2025 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

void mdio_init(void);
uint16_t mdio_read(uint8_t phy, uint8_t reg);
void mdio_write(uint8_t phy, uint8_t reg, uint16_t data);