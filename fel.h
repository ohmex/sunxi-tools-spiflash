/*
 * Copyright (C) 2012      Henrik Nordstrom <henrik@henriknordstrom.net>
 * Copyright (C) 2015-2016 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SUNXI_TOOLS_FEL_H
#define _SUNXI_TOOLS_FEL_H

#include <libusb.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * The 'sram_swap_buffers' structure is used to describe information about
 * two buffers in SRAM, the content of which needs to be exchanged before
 * calling the U-Boot SPL code and then exchanged again before returning
 * control back to the FEL code from the BROM.
 */

typedef struct {
	uint32_t buf1; /* BROM buffer */
	uint32_t buf2; /* backup storage location */
	uint32_t size; /* buffer size */
} sram_swap_buffers;

/*
 * Each SoC variant may have its own list of memory buffers to be exchanged
 * and the information about the placement of the thunk code, which handles
 * the transition of execution from the BROM FEL code to the U-Boot SPL and
 * back.
 *
 * Note: the entries in the 'swap_buffers' tables need to be sorted by 'buf1'
 * addresses. And the 'buf1' addresses are the BROM data buffers, while 'buf2'
 * addresses are the intended backup locations.
 *
 * Also for performance reasons, we optionally want to have MMU enabled with
 * optimal section attributes configured (the code from the BROM should use
 * I-cache, writing data to the DRAM area should use write combining). The
 * reason is that the BROM FEL protocol implementation moves data using the
 * CPU somewhere on the performance critical path when transferring data over
 * USB. The older SoC variants (A10/A13/A20/A31/A23) already have MMU enabled
 * and we only need to adjust section attributes. The BROM in newer SoC variants
 * (A33/A83T/H3) doesn't enable MMU anymore, so we need to find some 16K of
 * spare space in SRAM to place the translation table there and specify it as
 * the 'mmu_tt_addr' field in the 'soc_sram_info' structure. The 'mmu_tt_addr'
 * address must be 16K aligned.
 */
typedef struct {
	uint32_t           soc_id;       /* ID of the SoC */
	uint32_t           spl_addr;     /* SPL load address */
	uint32_t           scratch_addr; /* A safe place to upload & run code */
	uint32_t           thunk_addr;   /* Address of the thunk code */
	uint32_t           thunk_size;   /* Maximal size of the thunk code */
	bool               needs_l2en;   /* Set the L2EN bit */
	uint32_t           mmu_tt_addr;  /* MMU translation table address */
	uint32_t           sid_addr;     /* base address for SID_KEY[0-3] registers */
	sram_swap_buffers *swap_buffers;
} soc_sram_info;

void aw_fel_read(libusb_device_handle *usb, uint32_t offset, void *buf, size_t len);
void aw_fel_write(libusb_device_handle *usb, void *buf, uint32_t offset, size_t len);
void aw_fel_execute(libusb_device_handle *usb, uint32_t offset);
uint32_t aw_fel_readl(libusb_device_handle *usb, uint32_t addr);
void aw_fel_writel(libusb_device_handle *usb, uint32_t addr, uint32_t val);
soc_sram_info *aw_fel_get_sram_info(libusb_device_handle *usb);
void hexdump(void *data, uint32_t offset, size_t size);

bool aw_fel_remotefunc_prepare(libusb_device_handle *usb,
			       size_t                stack_size,
			       void                 *arm_code,
			       size_t                arm_code_size,
			       size_t                num_args,
			       uint32_t             *args);
bool aw_fel_remotefunc_execute(libusb_device_handle *usb, uint32_t *result);

#endif
