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

#include "vdphci_direct_io.h"
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/pagemap.h>

static int vdphci_direct_io_start(unsigned long start, size_t count, int write, struct page*** pages, int* num_pages)
{
    int ret = 0;
    *num_pages = PAGE_ALIGN((start & ~PAGE_MASK) + count) >> PAGE_SHIFT;
    *pages = kmalloc((*num_pages) * sizeof(struct page*), GFP_KERNEL);

    if (!(*pages)) {
        return -ENOMEM;
    }

    down_read(&current->mm->mmap_sem);

    ret = get_user_pages(current, current->mm, start,
        (*num_pages), write, 0, *pages, NULL);

    up_read(&current->mm->mmap_sem);

    if (ret < (*num_pages)) {
        int i;

        for (i = 0; i < ret; ++i) {
            put_page((*pages)[i]);
        }
        kfree(*pages);

        return -ENOMEM;
    }

    return 0;
}

static void vdphci_direct_io_end(struct page** pages, int num_pages, int dirty)
{
    int i;

    for (i = 0; i < num_pages; i++) {
        if (dirty) {
            set_page_dirty_lock(pages[i]);
        }

        put_page(pages[i]);
    }

    kfree(pages);
}

int vdphci_direct_read_start(const char __user* buf, size_t count, struct page*** pages, int* num_pages)
{
    BUG_ON(in_atomic());

    return vdphci_direct_io_start((unsigned long)buf, count, 0, pages, num_pages);
}

int vdphci_direct_write_start(char __user* buf, size_t count, struct page*** pages, int* num_pages)
{
    BUG_ON(in_atomic());

    return vdphci_direct_io_start((unsigned long)buf, count, 1, pages, num_pages);
}

void vdphci_direct_read_end(struct page** pages, int num_pages)
{
    BUG_ON(in_atomic());

    vdphci_direct_io_end(pages, num_pages, 0);
}

void vdphci_direct_write_end(struct page** pages, int num_pages)
{
    BUG_ON(in_atomic());

    vdphci_direct_io_end(pages, num_pages, 1);
}

int vdphci_direct_read(void* dst, size_t count, size_t offset, const char __user* buf, struct page** pages)
{
    size_t start_offset = (unsigned long)buf & (PAGE_SIZE - 1);

    while (count > 0) {
        size_t page_offset = (start_offset + offset) & (PAGE_SIZE - 1);
        size_t num_transfer = min((size_t)PAGE_SIZE - page_offset, count);
        void* vaddr = kmap_atomic(pages[(start_offset + offset) >> PAGE_SHIFT]);

        if (vaddr == NULL) {
            return -ENOMEM;
        }

        memcpy(dst, vaddr + page_offset, num_transfer);

        kunmap_atomic(vaddr);

        offset += num_transfer;
        dst += num_transfer;
        count -= num_transfer;
    }

    return 0;
}

int vdphci_direct_write(size_t offset, size_t count, const void* src, char __user* buf, struct page** pages)
{
    size_t start_offset = (unsigned long)buf & (PAGE_SIZE - 1);

    while (count > 0) {
        size_t page_offset = (start_offset + offset) & (PAGE_SIZE - 1);
        size_t num_transfer = min((size_t)PAGE_SIZE - page_offset, count);
        void* vaddr = kmap_atomic(pages[(start_offset + offset) >> PAGE_SHIFT]);

        if (vaddr == NULL) {
            return -ENOMEM;
        }

        memcpy(vaddr + page_offset, src, num_transfer);

        kunmap_atomic(vaddr);

        offset += num_transfer;
        src = ((const char*)src) + num_transfer;
        count -= num_transfer;
    }

    return 0;
}
