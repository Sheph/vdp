#ifndef _VDPHCI_DEVICE_H_
#define _VDPHCI_DEVICE_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

struct vdphci_hcd;

struct vdphci_port;

struct vdphci_device
{
    /*
     * HCD that created us.
     */

    struct vdphci_hcd* parent_hcd;

    /*
     * Port this device is connected to
     */

    struct vdphci_port* port;

    /*
     * Char device representing this device.
     * @{
     */
    struct cdev cdev;
    struct mutex cdev_mutex;

    /*
     * We only allow one opened file at a time. Note that opening a device doesn't mean
     * being ready to work with it, user might wanted to just check if device is busy or not,
     * device shouldn't report itself as connected until 'attached' is true.
     */
    int opened;
    /*
     * @}
     */
};

static inline struct vdphci_device* cdev_to_vdphci_device(struct cdev* dev)
{
    return container_of((void*)dev, struct vdphci_device, cdev);
}

int vdphci_device_init(struct vdphci_hcd* parent_hcd, struct vdphci_port* port, dev_t devno, struct vdphci_device* device);

void vdphci_device_cleanup(struct vdphci_device* device);

#endif
