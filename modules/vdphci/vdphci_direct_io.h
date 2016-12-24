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

int vdphci_direct_read(char* dst, size_t count, size_t offset, const char __user* buf, struct page** pages);

int vdphci_direct_write(size_t offset, size_t count, const void* src, char __user* buf, struct page** pages);

/*
 * @}
 */

#endif
