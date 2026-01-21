/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _REALTEK_SMI_H
#define _REALTEK_SMI_H

int realtek_smi_read_reg(uint32_t addr, uint32_t *data);
int realtek_smi_write_reg(uint32_t addr, uint32_t data, bool ack);

#endif  /* _REALTEK_SMI_H */
