#include <linux/poll.h>
#include "debug.h"
#include "print.h"
#include "vdphci_device.h"
#include "vdphci_port.h"
#include "vdphci_hcd.h"
#include "vdphci_direct_io.h"

static int vdphci_device_translate_urb_status(vdphci_urb_status status, int* res)
{
    switch (status) {
    case vdphci_urb_status_completed:
        *res = 0;
        break;
    case vdphci_urb_status_unlinked:
        *res = -ENOENT;
        break;
    case vdphci_urb_status_error:
        *res = -EPROTO;
        break;
    case vdphci_urb_status_stall:
        *res = -EPIPE;
        break;
    case vdphci_urb_status_overflow:
        *res = -EOVERFLOW;
        break;
    case vdphci_urb_status_unprocessed:
    default:
        return 0;
    }

    return 1;
}

/*
 * 'cdev_mutex' must be held
 */
static int vdphci_device_attach_nolock(struct vdphci_device* device, int attach)
{
    unsigned long flags;
    int need_invalidate = 0;
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    dprintk("enter\n");

    attach = !!attach;

    vdphci_hcd_lock(device->parent_hcd, flags);
    if (attach != vdphci_port_is_device_attached(device->port)) {
        vdphci_port_set_device_attached(device->port, attach);
        vdphci_port_update(device->port, &giveback_list);
        need_invalidate = 1;
    }
    vdphci_hcd_unlock(device->parent_hcd, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    if (need_invalidate) {
        vdphci_hcd_invalidate_ports(device->parent_hcd);

        if (attach) {
            dprintk("%s, device %d: attached\n",
                vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
                (int)device->port->number);
        } else {
            dprintk("%s, device %d: detached\n",
                vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
                (int)device->port->number);
        }
    }

    dprintk("exit\n");

    return 0;
}

static int vdphci_device_open(struct inode* inode, struct file* file)
{
    struct vdphci_device* device = cdev_to_vdphci_device(inode->i_cdev);

    BUG_ON(in_atomic());

    if (mutex_lock_interruptible(&device->cdev_mutex)) {
        return -ERESTARTSYS;
    }

    if (file->f_mode & FMODE_EXEC) {
        mutex_unlock(&device->cdev_mutex);

        return -EPERM;
    }

    if (device->opened) {
        mutex_unlock(&device->cdev_mutex);

        return -EBUSY;
    }

    file->private_data = device;

    device->opened = 1;

    mutex_unlock(&device->cdev_mutex);

    dprintk("%s, device %d: file %p opened\n",
        vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
        (int)device->port->number,
        file);

    return nonseekable_open(inode, file);
}

static int vdphci_device_release(struct inode* inode, struct file* file)
{
    struct vdphci_device* device = cdev_to_vdphci_device(inode->i_cdev);

    BUG_ON(in_atomic());

    mutex_lock(&device->cdev_mutex);

    vdphci_device_attach_nolock(device, 0);

    dprintk("%s, device %d: file %p closed\n",
        vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
        (int)device->port->number,
        file);

    device->opened = 0;

    mutex_unlock(&device->cdev_mutex);

    return 0;
}

/*
 * count is >= sizeof(struct vdphci_devent_header)
 * @{
 */

static int vdphci_device_process_signal_devent(struct vdphci_device* device, const char __user* buf,
    struct page** pages,
    size_t count)
{
    struct vdphci_devent_signal signal_devent;
    int retval = 0;

    if (count != (sizeof(struct vdphci_devent_header) + sizeof(signal_devent))) {
        return -EINVAL;
    }

    retval = vdphci_direct_read(&signal_devent, sizeof(signal_devent),
        sizeof(struct vdphci_devent_header), buf, pages);

    if (retval != 0) {
        return retval;
    }

    switch (signal_devent.signal) {
    case vdphci_dsignal_attached: {
        retval = vdphci_device_attach_nolock(device, 1);
        break;
    }
    case vdphci_dsignal_detached: {
        retval = vdphci_device_attach_nolock(device, 0);
        break;
    }
    default:
        retval = -EINVAL;
        break;
    }

    return retval;
}

/*
 * URB writing routines, i.e they're called when the user wants to complete an URB.
 * The 'count' is the number of bytes to hold THE EVENT BUFFER.
 * The return value is either "< 0 error" or "0 success".
 * @{
 */

static int vdphci_device_write_in_other_urb(const struct vdphci_devent_urb* urb_devent,
    struct urb* urb,
    const char __user* buf,
    struct page** pages,
    size_t count)
{
    int retval = 0;

    if (count > urb->transfer_buffer_length) {
        return -EINVAL;
    }

    if (count != urb_devent->actual_length) {
        return -EINVAL;
    }

    retval = vdphci_direct_read(urb->transfer_buffer, count,
        sizeof(struct vdphci_devent_header) + offsetof(struct vdphci_devent_urb, data.buff),
        buf, pages);

    if (retval != 0) {
        return retval;
    }

    if (!vdphci_device_translate_urb_status(urb_devent->status, &urb->status)) {
        return -EINVAL;
    }

    urb->actual_length = count;

    return 0;
}

static int vdphci_device_write_out_control_urb(const struct vdphci_devent_urb* urb_devent,
    struct urb* urb,
    const char __user* buf,
    struct page** pages,
    size_t count)
{
    if (urb_devent->actual_length > urb->transfer_buffer_length) {
        return -EINVAL;
    }

    if (count != 0) {
        return -EINVAL;
    }

    if (!vdphci_device_translate_urb_status(urb_devent->status, &urb->status)) {
        return -EINVAL;
    }

    urb->actual_length = urb_devent->actual_length;

    return 0;
}

/*
 * @}
 */

static int vdphci_device_process_urb_devent(struct vdphci_device* device, const char __user* buf,
    struct page** pages,
    size_t count)
{
    struct vdphci_devent_urb urb_devent;
    int retval = 0;
    unsigned long flags;
    struct vdphci_khevent_urb* urb_khevent;
    struct list_head giveback_list;
    size_t event_data_size = count - sizeof(struct vdphci_devent_header) - offsetof(struct vdphci_devent_urb, data.buff);

    INIT_LIST_HEAD(&giveback_list);

    if (count < (sizeof(struct vdphci_devent_header) + offsetof(struct vdphci_devent_urb, data.buff))) {
        return -EINVAL;
    }

    retval = vdphci_direct_read(&urb_devent, offsetof(struct vdphci_devent_urb, data.buff),
        sizeof(struct vdphci_devent_header), buf, pages);

    if (retval != 0) {
        return retval;
    }

    vdphci_hcd_lock(device->parent_hcd, flags);

    urb_khevent = vdphci_port_khevent_urb_find(device->port, urb_devent.seq_num);

    if (!urb_khevent) {
        retval = 0;

        goto out;
    }

    if (usb_pipein(urb_khevent->urb->pipe)) {
        switch (usb_pipetype(urb_khevent->urb->pipe)) {
        case PIPE_CONTROL:
        case PIPE_BULK:
        case PIPE_INTERRUPT:
            retval = vdphci_device_write_in_other_urb(&urb_devent,
                urb_khevent->urb,
                buf,
                pages,
                event_data_size);
            break;
        case PIPE_ISOCHRONOUS:
        default:
            print_error("Bad URB pipe type: %d\n", usb_pipetype(urb_khevent->urb->pipe));
            urb_khevent->urb->status = -EPROTO;
            retval = 0;
        }
    } else {
        switch (usb_pipetype(urb_khevent->urb->pipe)) {
        case PIPE_CONTROL: {
            retval = vdphci_device_write_out_control_urb(&urb_devent,
                urb_khevent->urb,
                buf,
                pages,
                event_data_size);
            break;
        }
        case PIPE_BULK:
        case PIPE_INTERRUPT:
        case PIPE_ISOCHRONOUS:
        default:
            print_error("Bad URB pipe type: %d\n", usb_pipetype(urb_khevent->urb->pipe));
            urb_khevent->urb->status = -EPROTO;
            retval = 0;
        }
    }

    if (retval >= 0) {
        vdphci_port_khevent_urb_remove(device->port, urb_khevent, &giveback_list);
    }

out:
    vdphci_hcd_unlock(device->parent_hcd, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    return retval;
}

/*
 * @}
 */

/*
 * Called with HCD lock being held. Also, count is >= sizeof(struct vdphci_hevent_header)
 * @{
 */

static int vdphci_device_process_signal_hevent(
    const struct vdphci_khevent_signal* event,
    char __user* buf,
    struct page** pages,
    size_t count)
{
    int retval = 0;
    struct vdphci_hevent_header uheader;
    struct vdphci_hevent_signal uevent;

    BUG_ON(count < sizeof(uheader));

    uheader.type = vdphci_hevent_type_signal;
    uheader.length = sizeof(uevent);

    retval = vdphci_direct_write(0, sizeof(uheader), &uheader, buf, pages);

    if (retval != 0) {
        return retval;
    }

    if (count < (sizeof(uheader) + sizeof(uevent))) {
        return sizeof(uheader);
    }

    uevent.signal = event->signal;

    retval = vdphci_direct_write(sizeof(uheader), sizeof(uevent), &uevent, buf, pages);

    if (retval != 0) {
        return retval;
    }

    return sizeof(uheader) + sizeof(uevent);
}

static int vdphci_urb_hevent_write_common(u32 seq_num,
    struct urb* urb,
    char __user* buf,
    struct page** pages)
{
    struct vdphci_hevent_urb data;

    data.seq_num = seq_num;

    switch (usb_pipetype(urb->pipe)) {
    default: {
        BUG();
    }
    case PIPE_CONTROL: {
        data.type = vdphci_urb_type_control;
        break;
    }
    case PIPE_BULK: {
        data.type = vdphci_urb_type_bulk;
        break;
    }
    case PIPE_INTERRUPT: {
        data.type = vdphci_urb_type_int;
        break;
    }
    case PIPE_ISOCHRONOUS:
        data.type = vdphci_urb_type_iso;
        data.number_of_packets = urb->number_of_packets;
        break;
    }

    data.flags = 0;

    if (urb->transfer_flags & URB_ZERO_PACKET) {
        data.flags |= VDPHCI_URB_ZERO_PACKET;
    }

    data.endpoint_address = urb->ep->desc.bEndpointAddress;

    if (usb_pipein(urb->pipe)) {
        data.endpoint_address |= USB_DIR_IN;
    }

    data.transfer_length = urb->transfer_buffer_length;

    if ((usb_pipetype(urb->pipe) == PIPE_INTERRUPT) ||
        (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS)) {
        data.interval = urb->interval;

        if (urb->dev->speed == USB_SPEED_LOW ||
            urb->dev->speed == USB_SPEED_FULL) {
            data.interval *= 1000;
        } else {
            data.interval *= 125;
        }
    }

    return vdphci_direct_write(sizeof(struct vdphci_hevent_header),
        offsetof(struct vdphci_hevent_urb, data.buff),
        &data,
        buf,
        pages);
}

/*
 * URB reading routines, i.e they're called when the user wants to get the next available URB.
 * The 'count' is the number of bytes to hold THE EVENT DATA, WITHOUT THE HEADER.
 * The return value is either "< 0 error" or "> 0 success", in case of success
 * the're 2 cases:
 * + the return value is > count: in this case the routine shouldn't write anything
 *   to user's pages, the caller will write the header with returned length itself in order
 *   to notify the user that the buffer that he specified isn't sufficient to hold the event.
 * + the return value is <= count: in this case the routine should have written the
 *   event data at the proper offset, so the caller will write the header and return.
 * @{
 */

static int vdphci_device_read_in_control_urb(
    u32 seq_num,
    struct urb* urb,
    char __user* buf,
    struct page** pages,
    size_t count)
{
    int retval = 0;
    size_t data_size = offsetof(struct vdphci_hevent_urb, data.buff) +
        sizeof(struct usb_ctrlrequest);

    if (count < data_size) {
        return data_size;
    }

    retval = vdphci_urb_hevent_write_common(seq_num, urb, buf, pages);

    if (retval != 0) {
        return retval;
    }

    retval = vdphci_direct_write(sizeof(struct vdphci_hevent_header) +
        offsetof(struct vdphci_hevent_urb, data.buff),
        sizeof(struct usb_ctrlrequest),
        urb->setup_packet,
        buf,
        pages);

    if (retval != 0) {
        return retval;
    }

    return data_size;
}

static int vdphci_device_read_in_other_urb(
    u32 seq_num,
    struct urb* urb,
    char __user* buf,
    struct page** pages,
    size_t count)
{
    int retval = 0;
    size_t data_size = offsetof(struct vdphci_hevent_urb, data.buff);

    if (count < data_size) {
        return data_size;
    }

    retval = vdphci_urb_hevent_write_common(seq_num, urb, buf, pages);

    if (retval != 0) {
        return retval;
    }

    return data_size;
}

static int vdphci_device_read_out_control_urb(
    u32 seq_num,
    struct urb* urb,
    char __user* buf,
    struct page** pages,
    size_t count)
{
    int retval = 0;
    size_t data_size = offsetof(struct vdphci_hevent_urb, data.buff) +
        sizeof(struct usb_ctrlrequest) + urb->transfer_buffer_length;

    if (count < data_size) {
        return data_size;
    }

    retval = vdphci_urb_hevent_write_common(seq_num, urb, buf, pages);

    if (retval != 0) {
        return retval;
    }

    retval = vdphci_direct_write(sizeof(struct vdphci_hevent_header) +
        offsetof(struct vdphci_hevent_urb, data.buff),
        sizeof(struct usb_ctrlrequest),
        urb->setup_packet,
        buf,
        pages);

    if (retval != 0) {
        return retval;
    }

    retval = vdphci_direct_write(sizeof(struct vdphci_hevent_header) +
        offsetof(struct vdphci_hevent_urb, data.buff) +
        sizeof(struct usb_ctrlrequest),
        urb->transfer_buffer_length,
        urb->transfer_buffer,
        buf,
        pages);

    if (retval != 0) {
        return retval;
    }

    return data_size;
}

/*
 * @}
 */

static int vdphci_device_process_urb_hevent(
    const struct vdphci_khevent_urb* event,
    char __user* buf,
    struct page** pages,
    size_t count)
{
    int retval = 0;
    int wr_retval = 0;
    struct vdphci_hevent_header uheader;
    size_t event_data_size = count - sizeof(uheader);

    BUG_ON(count < sizeof(uheader));

    if (usb_pipein(event->urb->pipe)) {
        switch (usb_pipetype(event->urb->pipe)) {
        case PIPE_CONTROL: {
            retval = vdphci_device_read_in_control_urb(event->seq_num,
                event->urb,
                buf,
                pages,
                event_data_size);
            break;
        }
        case PIPE_BULK:
        case PIPE_INTERRUPT:
            retval = vdphci_device_read_in_other_urb(event->seq_num,
                event->urb,
                buf,
                pages,
                event_data_size);
            break;
        case PIPE_ISOCHRONOUS:
        default:
            print_error("Bad URB pipe type: %d\n", usb_pipetype(event->urb->pipe));
            retval = -EIO;
        }
    } else {
        switch (usb_pipetype(event->urb->pipe)) {
        case PIPE_CONTROL: {
            retval = vdphci_device_read_out_control_urb(event->seq_num,
                event->urb,
                buf,
                pages,
                event_data_size);
            break;
        }
        case PIPE_BULK:
        case PIPE_INTERRUPT:
        case PIPE_ISOCHRONOUS:
        default:
            print_error("Bad URB pipe type: %d\n", usb_pipetype(event->urb->pipe));
            retval = -EIO;
        }
    }

    if (retval < 0) {
        return retval;
    }

    uheader.type = vdphci_hevent_type_urb;
    uheader.length = retval;

    wr_retval = vdphci_direct_write(0, sizeof(uheader), &uheader, buf, pages);

    if (wr_retval != 0) {
        return wr_retval;
    }

    if (retval > event_data_size) {
        return sizeof(uheader);
    } else {
        return sizeof(uheader) + retval;
    }
}

static int vdphci_device_process_unlink_urb_hevent(
    const struct vdphci_khevent_unlink_urb* event,
    char __user* buf,
    struct page** pages,
    size_t count)
{
    struct vdphci_hevent_header uheader;

    BUG_ON(count < sizeof(uheader));

    return -EINVAL;
}

static int vdphci_device_process_no_hevent(
    char __user* buf,
    struct page** pages,
    size_t count)
{
    struct vdphci_hevent_header uheader;

    BUG_ON(count < sizeof(uheader));

    return 0;
}

/*
 * @}
 */

static ssize_t vdphci_device_write(struct file* file, const char __user* buf, size_t count, loff_t* f_pos)
{
    struct vdphci_device* device = file->private_data;
    int retval = 0;
    struct page** pages;
    int num_pages;
    struct vdphci_devent_header header;

    dprintk("%s, device %d: write %d to file %p\n",
        vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
        (int)device->port->number,
        (int)count,
        file);

    if (mutex_lock_interruptible(&device->cdev_mutex) != 0) {
        return -ERESTARTSYS;
    }

    if (count < sizeof(header)) {
        retval = -EINVAL;

        goto fail;
    }

    retval = vdphci_direct_read_start(buf, count, &pages, &num_pages);

    if (retval != 0) {
        goto fail;
    }

    retval = vdphci_direct_read(&header, sizeof(header), 0, buf, pages);

    if (retval != 0) {
        vdphci_direct_read_end(pages, num_pages);

        goto fail;
    }

    switch (header.type) {
    case vdphci_devent_type_signal: {
        retval = vdphci_device_process_signal_devent(device, buf, pages, count);
        break;
    }
    case vdphci_devent_type_urb: {
        retval = vdphci_device_process_urb_devent(device, buf, pages, count);
        break;
    }
    default:
        retval = -EINVAL;
        break;
    }

    vdphci_direct_read_end(pages, num_pages);

    if (retval < 0) {
        goto fail;
    }

    *f_pos += count;

fail:
    mutex_unlock(&device->cdev_mutex);

    return retval;
}

static ssize_t vdphci_device_read(struct file* file, char __user* buf, size_t count, loff_t *f_pos)
{
    struct vdphci_device* device = file->private_data;
    unsigned long flags;
    struct vdphci_khevent* event = NULL;
    int retval = 0;
    int no_event = 0;
    struct page** pages;
    int num_pages;

    dprintk("%s, device %d: read %d from file %p\n",
        vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
        (int)device->port->number,
        (int)count,
        file);

    if (mutex_lock_interruptible(&device->cdev_mutex) != 0) {
        return -ERESTARTSYS;
    }

    if (count < sizeof(struct vdphci_hevent_header)) {
        retval = -EINVAL;

        goto fail;
    }

    retval = vdphci_direct_write_start(buf, count, &pages, &num_pages);

    if (retval != 0) {
        goto fail;
    }

    vdphci_hcd_lock(device->parent_hcd, flags);

    event = vdphci_port_khevent_current(device->port);

    if (event) {
        switch (event->type) {
        case vdphci_hevent_type_signal: {
            retval = vdphci_device_process_signal_hevent(
                (const struct vdphci_khevent_signal*)event,
                buf,
                pages,
                count);
            break;
        }
        case vdphci_hevent_type_urb: {
            retval = vdphci_device_process_urb_hevent(
                (const struct vdphci_khevent_urb*)event,
                buf,
                pages,
                count);
            break;
        }
        case vdphci_hevent_type_unlink_urb: {
            retval = vdphci_device_process_unlink_urb_hevent(
                (const struct vdphci_khevent_unlink_urb*)event,
                buf,
                pages,
                count);
            break;
        }
        default:
            BUG_ON(1);
            no_event = 1;
            break;
        }
    } else {
        no_event = 1;
    }

    if (no_event) {
        retval = vdphci_device_process_no_hevent(
            buf,
            pages,
            count);
    }

    if (retval > sizeof(struct vdphci_hevent_header)) {
        /*
         * Event has been processed, move on
         */

        vdphci_port_khevent_proceed(device->port);
    }

    vdphci_hcd_unlock(device->parent_hcd, flags);

    vdphci_direct_write_end(pages, num_pages);

    if (retval < 0) {
        goto fail;
    }

    BUG_ON(retval > count);

    *f_pos += retval;

fail:
    mutex_unlock(&device->cdev_mutex);

    return retval;
}

static unsigned int vdphci_device_poll(struct file* file, struct poll_table_struct* wait)
{
    int ret;
    unsigned long flags;
    struct vdphci_device* device = file->private_data;

    poll_wait(file, vdphci_port_get_khevent_wq(device->port), wait);

    vdphci_hcd_lock(device->parent_hcd, flags);

    ret = ((vdphci_port_khevent_current(device->port) != NULL) ? (POLLIN | POLLRDNORM) : 0);

    vdphci_hcd_unlock(device->parent_hcd, flags);

    ret |= (POLLOUT | POLLWRNORM);

    return ret;
}

static struct file_operations vdphci_device_ops =
{
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .open = vdphci_device_open,
    .release = vdphci_device_release,
    .write = vdphci_device_write,
    .read = vdphci_device_read,
    .poll = vdphci_device_poll
};

int vdphci_device_init(struct vdphci_hcd* parent_hcd, struct vdphci_port* port, dev_t devno, struct vdphci_device* device)
{
    int ret;

    BUG_ON(in_atomic());

    memset(device, 0, sizeof(*device));

    device->parent_hcd = parent_hcd;

    device->port = port;

    cdev_init(&device->cdev, &vdphci_device_ops);

    device->cdev.owner = THIS_MODULE;
    device->cdev.ops = &vdphci_device_ops;

    mutex_init(&device->cdev_mutex);

    device->opened = 0;

    /*
     * This should be our final step.
     */
    ret = cdev_add(&device->cdev, devno, 1);

    if (ret != 0) {
        dprintk("%s: error %d adding char device (%d, %d)\n",
            vdphci_hcd_to_usb_hcd(parent_hcd)->self.bus_name,
            ret,
            MAJOR(devno),
            MINOR(devno));
    } else {
        dprintk("%s: char device (%d, %d) created\n",
            vdphci_hcd_to_usb_hcd(parent_hcd)->self.bus_name,
            MAJOR(devno),
            MINOR(devno));
    }

    return ret;
}

void vdphci_device_cleanup(struct vdphci_device* device)
{
    BUG_ON(in_atomic());

    cdev_del(&device->cdev);

    dprintk("%s: char device (%d, %d) removed\n",
        vdphci_hcd_to_usb_hcd(device->parent_hcd)->self.bus_name,
        MAJOR(device->cdev.dev),
        MINOR(device->cdev.dev));
}
