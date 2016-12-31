#include "vdphci_direct_io.h"
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/pagemap.h>

static int vdphci_direct_io_start(unsigned long start, size_t count, int write, struct page*** pages, int* num_pages)
{
    int ret = 0;
    *num_pages = (count + PAGE_SIZE - 1) >> PAGE_SHIFT;
    *pages = kmalloc((*num_pages) * sizeof(struct page*), GFP_KERNEL);

    if (!(*pages)) {
        return -ENOMEM;
    }

    down_read(&current->mm->mmap_sem);

    ret = get_user_pages(current, current->mm, start,
        (*num_pages), write, 0, *pages, NULL);

    up_read(&current->mm->mmap_sem);

    if (ret < (*num_pages)) {
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

        page_cache_release(pages[i]);
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
