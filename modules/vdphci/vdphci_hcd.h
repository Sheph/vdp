#ifndef _VDPHCI_HCD_H_
#define _VDPHCI_HCD_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include "vdphci-common.h"
#include "vdphci_platform_driver.h"
#include "vdphci_device.h"
#include "vdphci_port.h"

struct vdphci_hcd
{
    /*
     * Fields below are set on 'vdphci_hcd_add' from platform data.
     */

    u8 num_ports;
    int major;

    /*
     * Fields below are set on HCD start and cleaned up on HCD stop, they use
     * the fields above.
     */

    spinlock_t lock;

    /*
     * 'bus_suspend' was called.
     */
    int suspended;

    /*
     * Begin of range of device numbers. Range is 'num_ports' long.
     */
    dev_t devno;

    /*
     * Virtual devices that user drives.
     */
    struct vdphci_device devices[VDPHCI_MAX_PORTS];

    /*
     * Virtual ports, managed by HCD.
     */
    struct vdphci_port ports[VDPHCI_MAX_PORTS];
};

#define vdphci_hcd_lock(hcd, flags) spin_lock_irqsave(&(hcd)->lock, flags)

#define vdphci_hcd_unlock(hcd, flags) spin_unlock_irqrestore(&(hcd)->lock, flags)

static inline struct vdphci_hcd* usb_hcd_to_vdphci_hcd(struct usb_hcd* hcd)
{
    return (struct vdphci_hcd *)(hcd->hcd_priv);
}

static inline struct usb_hcd* vdphci_hcd_to_usb_hcd(struct vdphci_hcd* hcd)
{
    return container_of((void*)hcd, struct usb_hcd, hcd_priv);
}

/*
 * Creates and adds new HCD to system. Sets driver data of 'controller' to '*hcd'.
 */
int vdphci_hcd_add(struct device* controller,
    const char* bus_name,
    const struct vdphci_platform_data* platform_data,
    struct usb_hcd** hcd);

void vdphci_hcd_remove(struct usb_hcd* hcd);

/*
 * Forces HCD to re-query port statuses.
 */
void vdphci_hcd_invalidate_ports(struct vdphci_hcd* hcd);

#endif
