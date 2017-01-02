/*
 * Copyright (c) 2017, Stanislav Vorobiov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VDPHCI_DIRECT_IO_H_
#define _VDPHCI_DIRECT_IO_H_

#include <linux/kernel.h>
#include <linux/mm.h>

/*
 * The following functions MUST be called from a non-atomic context
 * @{
 */

int vdphci_direct_read_start(const char __user* buf, size_t count, struct page*** pages, int* num_pages);

int vdphci_direct_write_start(char __user* buf, size_t count, struct page*** pages, int* num_pages);

void vdphci_direct_read_end(struct page** pages, int num_pages);

void vdphci_direct_write_end(struct page** pages, int num_pages);

/*
 * @}
 */

/*
 * The following functions can be called from an atomic context
 * @{
 */

int vdphci_direct_read(void* dst, size_t count, size_t offset, const char __user* buf, struct page** pages);

int vdphci_direct_write(size_t offset, size_t count, const void* src, char __user* buf, struct page** pages);

/*
 * @}
 */

#endif
