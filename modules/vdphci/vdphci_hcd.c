#include "vdphci-common.h"
#include "vdphci_hcd.h"
#include "debug.h"
#include "print.h"

#define VDPHCI_PORT_C_MASK \
    ((USB_PORT_STAT_C_CONNECTION |\
      USB_PORT_STAT_C_ENABLE |\
      USB_PORT_STAT_C_SUSPEND |\
      USB_PORT_STAT_C_OVERCURRENT |\
      USB_PORT_STAT_C_RESET) << 16)

static const char vdphci_hcd_name[] = VDPHCI_NAME "_hcd";

#ifdef DEBUG

#define VDPHCI_PORT_FEATURE_TO_STR(prefix, feature) \
    switch (feature) { \
    case USB_PORT_FEAT_CONNECTION: return prefix "USB_PORT_FEAT_CONNECTION"; \
    case USB_PORT_FEAT_ENABLE: return prefix "USB_PORT_FEAT_ENABLE"; \
    case USB_PORT_FEAT_SUSPEND: return prefix "USB_PORT_FEAT_SUSPEND"; \
    case USB_PORT_FEAT_OVER_CURRENT: return prefix "USB_PORT_FEAT_OVER_CURRENT"; \
    case USB_PORT_FEAT_RESET: return prefix "USB_PORT_FEAT_RESET"; \
    case USB_PORT_FEAT_L1: return prefix "USB_PORT_FEAT_L1"; \
    case USB_PORT_FEAT_POWER: return prefix "USB_PORT_FEAT_POWER"; \
    case USB_PORT_FEAT_LOWSPEED: return prefix "USB_PORT_FEAT_LOWSPEED"; \
    case USB_PORT_FEAT_C_CONNECTION: return prefix "USB_PORT_FEAT_C_CONNECTION"; \
    case USB_PORT_FEAT_C_ENABLE: return prefix "USB_PORT_FEAT_C_ENABLE"; \
    case USB_PORT_FEAT_C_SUSPEND: return prefix "USB_PORT_FEAT_C_SUSPEND"; \
    case USB_PORT_FEAT_C_OVER_CURRENT: return prefix "USB_PORT_FEAT_C_OVER_CURRENT"; \
    case USB_PORT_FEAT_C_RESET: return prefix "USB_PORT_FEAT_C_RESET"; \
    case USB_PORT_FEAT_TEST: return prefix "USB_PORT_FEAT_TEST"; \
    case USB_PORT_FEAT_INDICATOR: return prefix "USB_PORT_FEAT_INDICATOR"; \
    case USB_PORT_FEAT_C_PORT_L1: return prefix "USB_PORT_FEAT_C_PORT_L1"; \
    default: return prefix "USB_PORT_FEAT_XXX"; \
    }

static const char* vdphci_request_to_str(u16 typeReq, u16 wValue)
{
    switch (typeReq) {
    case ClearHubFeature: return "ClearHubFeature";
    case ClearPortFeature: {
        VDPHCI_PORT_FEATURE_TO_STR("ClearPortFeature:", wValue);
    }
    case GetHubDescriptor: return "GetHubDescriptor";
    case GetHubStatus: return "GetHubStatus";
    case GetPortStatus: return "GetPortStatus";
    case SetHubFeature: return "SetHubFeature";
    case SetPortFeature: {
        VDPHCI_PORT_FEATURE_TO_STR("SetPortFeature:", wValue);
    }
    default: return "UnknownRequest";
    }
}

static const char* vdphci_pipetype_to_str(unsigned int pipetype)
{
    switch (pipetype) {
    case PIPE_ISOCHRONOUS: return "iso";
    case PIPE_INTERRUPT: return "int";
    case PIPE_CONTROL: return "ctrl";
    case PIPE_BULK: return "bulk";
    default: return "unknown";
    }
}

static const char* vdphci_ctrl_request_type_direction_to_str(u8 bmRequestType)
{
    if (bmRequestType & USB_DIR_IN) {
        return "in";
    } else {
        return "out";
    }
}

static const char* vdphci_ctrl_request_type_type_to_str(u8 bmRequestType)
{
    switch (bmRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD: return "standard";
    case USB_TYPE_CLASS: return "class";
    case USB_TYPE_VENDOR: return "vendor";
    case USB_TYPE_RESERVED: return "reserved";
    default: return "unknown";
    }
}

static const char* vdphci_ctrl_request_type_recipient_to_str(u8 bmRequestType)
{
    switch (bmRequestType & USB_RECIP_MASK) {
    case USB_RECIP_DEVICE: return "device";
    case USB_RECIP_INTERFACE: return "interface";
    case USB_RECIP_ENDPOINT: return "endpoint";
    case USB_RECIP_OTHER: return "other";
    case USB_RECIP_PORT: return "port";
    case USB_RECIP_RPIPE: return "rpipe";
    default: return "unknown";
    }
}

static const char* vdphci_ctrl_request_to_str(u8 bRequest)
{
    switch (bRequest) {
    case USB_REQ_GET_STATUS: return "USB_REQ_GET_STATUS";
    case USB_REQ_CLEAR_FEATURE: return "USB_REQ_CLEAR_FEATURE";
    case USB_REQ_SET_FEATURE: return "USB_REQ_SET_FEATURE";
    case USB_REQ_SET_ADDRESS: return "USB_REQ_SET_ADDRESS";
    case USB_REQ_GET_DESCRIPTOR: return "USB_REQ_GET_DESCRIPTOR";
    case USB_REQ_SET_DESCRIPTOR: return "USB_REQ_SET_DESCRIPTOR";
    case USB_REQ_GET_CONFIGURATION: return "USB_REQ_GET_CONFIGURATION";
    case USB_REQ_SET_CONFIGURATION: return "USB_REQ_SET_CONFIGURATION";
    case USB_REQ_GET_INTERFACE: return "USB_REQ_GET_INTERFACE";
    case USB_REQ_SET_INTERFACE: return "USB_REQ_SET_INTERFACE";
    case USB_REQ_SYNCH_FRAME: return "USB_REQ_SYNCH_FRAME";
    case USB_REQ_SET_ENCRYPTION: return "USB_REQ_SET_ENCRYPTION";
    case USB_REQ_GET_ENCRYPTION: return "USB_REQ_GET_ENCRYPTION";
    case USB_REQ_SET_HANDSHAKE: return "USB_REQ_SET_HANDSHAKE";
    case USB_REQ_GET_HANDSHAKE: return "USB_REQ_GET_HANDSHAKE";
    case USB_REQ_SET_CONNECTION: return "USB_REQ_SET_CONNECTION";
    case USB_REQ_SET_SECURITY_DATA: return "USB_REQ_SET_SECURITY_DATA";
    case USB_REQ_GET_SECURITY_DATA: return "USB_REQ_GET_SECURITY_DATA";
    case USB_REQ_SET_WUSB_DATA: return "USB_REQ_SET_WUSB_DATA";
    case USB_REQ_LOOPBACK_DATA_WRITE: return "USB_REQ_LOOPBACK_DATA_WRITE";
    case USB_REQ_LOOPBACK_DATA_READ: return "USB_REQ_LOOPBACK_DATA_READ";
    case USB_REQ_SET_INTERFACE_DS: return "USB_REQ_SET_INTERFACE_DS";
    default : return "USB_REQ_XXX";
    }
}

#endif

static inline void vdphci_fill_hub_descriptor(struct vdphci_hcd* hcd,
    struct usb_hub_descriptor* desc)
{
    memset(desc, 0, sizeof(*desc));

    desc->bDescriptorType = USB_DT_HUB;
    desc->bDescLength = 9;
    desc->wHubCharacteristics = cpu_to_le16(HUB_CHAR_INDV_PORT_LPSM |
        HUB_CHAR_COMMON_OCPM);
    desc->bNbrPorts = hcd->num_ports;
    desc->u.hs.DeviceRemovable[0] = 0xff;
    desc->u.hs.DeviceRemovable[1] = 0xff;
}

static int vdphci_register_chrdevs(struct vdphci_hcd* hcd)
{
    int cnt, ret = 0;
    dev_t devno = 0;
    u8 next_minor = 0;

    for (cnt = 0, ret = -EBUSY; (ret == -EBUSY) && (cnt <= 255); ++next_minor, ++cnt) {
        if (hcd->major) {
            devno = MKDEV(hcd->major, next_minor);

            ret = register_chrdev_region(devno, hcd->num_ports, VDPHCI_NAME);
        } else {
            ret = alloc_chrdev_region(&devno, next_minor, hcd->num_ports, VDPHCI_NAME);
        }
    }

    if (ret != 0) {
        hcd->devno = 0;

        dprintk("%s: can't register %d char devices for major %d\n",
            vdphci_hcd_to_usb_hcd(hcd)->self.bus_name,
            hcd->num_ports,
            hcd->major);
    } else {
        hcd->devno = devno;
    }

    return ret;
}

static void vdphci_unregister_chrdevs(struct vdphci_hcd* hcd)
{
    unregister_chrdev_region(hcd->devno, hcd->num_ports);
}

static int vdphci_start(struct usb_hcd* uhcd)
{
    int ret, devices_inited;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);

    dprintk("starting\n");

    spin_lock_init(&hcd->lock);

    ret = vdphci_register_chrdevs(hcd);

    if (ret != 0) {
        goto fail1;
    }

    /*
     * Initialize devices.
     */

    for (devices_inited = 0; devices_inited < hcd->num_ports; ++devices_inited) {
        dev_t devno = MKDEV(MAJOR(hcd->devno), MINOR(hcd->devno) + devices_inited);

        vdphci_port_init(devices_inited, &hcd->ports[devices_inited]);

        ret = vdphci_device_init(hcd, &hcd->ports[devices_inited], devno, &hcd->devices[devices_inited]);

        if (ret != 0) {
            vdphci_port_cleanup(&hcd->ports[devices_inited]);

            goto fail2;
        }
    }

    /*
     * Enter running state.
     */

    uhcd->power_budget = 0; /* no limit */
    uhcd->uses_new_polling = 1;

    dprintk("started\n");

    return 0;

fail2:
    while (devices_inited-- > 0) {
        vdphci_device_cleanup(&hcd->devices[devices_inited]);

        vdphci_port_cleanup(&hcd->ports[devices_inited]);
    }
    vdphci_unregister_chrdevs(hcd);
fail1:
    dprintk("start failed\n");

    return ret;
}

static void vdphci_stop(struct usb_hcd* uhcd)
{
    int i;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);

    dprintk("stopping\n");

    for (i = hcd->num_ports; i > 0; --i) {
        vdphci_device_cleanup(&hcd->devices[i - 1]);

        vdphci_port_cleanup(&hcd->ports[i - 1]);
    }

    vdphci_unregister_chrdevs(hcd);

    dprintk("stopped\n");
}

static int vdphci_urb_enqueue(struct usb_hcd* uhcd,
    struct urb* urb,
    gfp_t mem_flags)
{
    unsigned long flags;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);
    int ret = 0;
    struct vdphci_port* port;
    u32 seq_num = 0;

    dprintk("enter\n");

    BUG_ON(!urb->transfer_buffer && urb->transfer_buffer_length);

    spin_lock_irqsave(&hcd->lock, flags);

    if (urb->status != -EINPROGRESS) {
        print_error("%s: URB already unlinked!, status %d\n", uhcd->self.bus_name, urb->status);

        spin_unlock_irqrestore(&hcd->lock, flags);

        return urb->status;
    }

    if ((urb->dev->portnum > hcd->num_ports) || (urb->dev->portnum < 1)) {
        print_error("%s: bad portnum %d\n", uhcd->self.bus_name, urb->dev->portnum);

        spin_unlock_irqrestore(&hcd->lock, flags);

        return -EINVAL;
    }

    port = &hcd->ports[urb->dev->portnum - 1];

    if (!vdphci_port_is_enabled(port) || !HC_IS_RUNNING(uhcd->state)) {
        print_error("%s: port %d not enabled\n",
            uhcd->self.bus_name,
            port->number);

        spin_unlock_irqrestore(&hcd->lock, flags);

        return -ENODEV;
    }

    ret = usb_hcd_link_urb_to_ep(uhcd, urb);

    if (ret != 0) {
        print_error("%s: cannot link URB to EP: %d\n", uhcd->self.bus_name, ret);

        goto fail1;
    }

    ret = vdphci_port_urb_enqueue(port, urb, &seq_num);

    if (ret != 0) {
        print_error("%s: cannot add URB to port: %d\n", uhcd->self.bus_name, ret);

        goto fail2;
    }

#ifdef DEBUG
    dprintk("%s: seq_num = %u, port = %d, type = %s, dev = %d, ep = %d, dir = %s\n",
        uhcd->self.bus_name,
        seq_num,
        port->number,
        vdphci_pipetype_to_str(usb_pipetype(urb->pipe)),
        usb_pipedevice(urb->pipe),
        usb_pipeendpoint(urb->pipe),
        (usb_pipein(urb->pipe) ? "in" : "out"));

    if (usb_pipetype(urb->pipe) == PIPE_CONTROL) {
        struct usb_ctrlrequest* req = (struct usb_ctrlrequest*)urb->setup_packet;

        BUG_ON(!req);

        if ((req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
            dprintk("%s: bmRequestType = %s:%s:%s, bRequest = %s, wValue = %d, wIndex = %d, buff = (%d)\n",
                uhcd->self.bus_name,
                vdphci_ctrl_request_type_direction_to_str(req->bRequestType),
                vdphci_ctrl_request_type_type_to_str(req->bRequestType),
                vdphci_ctrl_request_type_recipient_to_str(req->bRequestType),
                vdphci_ctrl_request_to_str(req->bRequest),
                le16_to_cpu(req->wValue),
                le16_to_cpu(req->wIndex),
                le16_to_cpu(req->wLength));
        } else {
            dprintk("%s: bmRequestType = %s:%s:%s, bRequest = 0x%X, wValue = %d, wIndex = %d, buff = (%d)\n",
                uhcd->self.bus_name,
                vdphci_ctrl_request_type_direction_to_str(req->bRequestType),
                vdphci_ctrl_request_type_type_to_str(req->bRequestType),
                vdphci_ctrl_request_type_recipient_to_str(req->bRequestType),
                (int)req->bRequest,
                le16_to_cpu(req->wValue),
                le16_to_cpu(req->wIndex),
                le16_to_cpu(req->wLength));
        }
    }
#endif

    spin_unlock_irqrestore(&hcd->lock, flags);

    dprintk("exit\n");

    return 0;

fail2:
    usb_hcd_unlink_urb_from_ep(uhcd, urb);

fail1:
    spin_unlock_irqrestore(&hcd->lock, flags);

    usb_hcd_giveback_urb(uhcd, urb, urb->status);

    return ret;
}

static int vdphci_urb_dequeue(struct usb_hcd* uhcd, struct urb* urb, int status)
{
    unsigned long flags;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);
    int ret = 0;
    struct vdphci_port* port;
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    dprintk("enter\n");

    spin_lock_irqsave(&hcd->lock, flags);

    if ((urb->dev->portnum > hcd->num_ports) || (urb->dev->portnum < 1)) {
        print_error("%s: bad portnum %d\n", uhcd->self.bus_name, urb->dev->portnum);

        spin_unlock_irqrestore(&hcd->lock, flags);

        return -EINVAL;
    }

    port = &hcd->ports[urb->dev->portnum - 1];

    ret = usb_hcd_check_unlink_urb(uhcd, urb, status);

    if (ret != 0) {
        spin_unlock_irqrestore(&hcd->lock, flags);

        return ret;
    }

    vdphci_port_urb_dequeue(port, urb, &giveback_list);

    spin_unlock_irqrestore(&hcd->lock, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    dprintk("exit\n");

    return 0;
}

static int vdphci_get_frame_number(struct usb_hcd* hcd)
{
    struct timeval tv;

    do_gettimeofday(&tv);

    return tv.tv_usec / 1000;
}

static int vdphci_hub_status_data(struct usb_hcd* uhcd, char* buf)
{
    unsigned long flags;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);
    u32* event_bits = (u32*)buf;
    u8 port_number;
    int ret = 0;
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    dprintk("enter\n");

    spin_lock_irqsave(&hcd->lock, flags);

    if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &uhcd->flags)) {
        spin_unlock_irqrestore(&hcd->lock, flags);

        return 0;
    }

    for (port_number = 0; port_number < hcd->num_ports; ++port_number) {
        struct vdphci_port* port = &hcd->ports[port_number];

        if (vdphci_port_is_resuming(port) &&
            time_after(jiffies, vdphci_port_get_re_timeout(port))) {
            vdphci_port_set_status_bits(port, USB_PORT_STAT_C_SUSPEND << 16);
            vdphci_port_reset_status_bits(port, USB_PORT_STAT_SUSPEND);
            vdphci_port_update(port, &giveback_list);
        }

        if (vdphci_port_check_status_bits(port, VDPHCI_PORT_C_MASK)) {
#ifdef DEBUG
            char* status_str = vdphci_port_status_to_str(vdphci_port_get_status(port));
            if (status_str) {
                dprintk("%s: port %d status changed to: %s\n",
                    uhcd->self.bus_name,
                    port_number,
                    status_str);
                vdphci_port_free_status_str(status_str);
            }
#endif
            BUG_ON(port_number >= (sizeof(*event_bits)*8 - 1));
            *event_bits |= 1 << (port_number + 1);
            ret = 1;
        }
    }

    if ((ret == 1) && hcd->suspended) {
        dprintk("%s: resuming root hub\n", uhcd->self.bus_name);
        usb_hcd_resume_root_hub(uhcd);
    }

    spin_unlock_irqrestore(&hcd->lock, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    dprintk("exit\n");

    return ret;
}

static int vdphci_hub_control(struct usb_hcd* uhcd,
    u16 typeReq,
    u16 wValue,
    u16 wIndex,
    char* buf,
    u16 wLength)
{
    unsigned long flags;
    int ret = 0;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);
    struct vdphci_port* port;
    int need_invalidate = 0;
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    dprintk("enter\n");

    dprintk("%s: %s wValue = 0x%X, wIndex = 0x%X, buff = (%d)\n",
        uhcd->self.bus_name,
        vdphci_request_to_str(typeReq, wValue),
        wValue,
        wIndex,
        wLength);

    port = &hcd->ports[wIndex - 1];

    spin_lock_irqsave(&hcd->lock, flags);

    if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &uhcd->flags)) {
        spin_unlock_irqrestore(&hcd->lock, flags);

        return -ETIMEDOUT;
    }

    switch (typeReq) {
    case ClearHubFeature: {
        break;
    }
    case ClearPortFeature: {
        if (wIndex > hcd->num_ports || wIndex < 1) {
            print_error("%s: invalid port number %d\n",
                uhcd->self.bus_name,
                wIndex);
            ret = -EPIPE;
            break;
        }

        switch (wValue) {
        case USB_PORT_FEAT_SUSPEND: {
            if (vdphci_port_check_status_bits(port, USB_PORT_STAT_SUSPEND)) {
                vdphci_port_set_resuming(port, 1);
                vdphci_port_set_re_timeout(port, 20);
            }
            break;
        }
        default:
            vdphci_port_reset_status_bits(port, 1 << wValue);
            vdphci_port_update(port, &giveback_list);
        }

        break;
    }
    case GetHubDescriptor: {
        vdphci_fill_hub_descriptor(hcd, (struct usb_hub_descriptor*)buf);
        break;
    }
    case GetHubStatus: {
        *(u32*)buf = 0;
        break;
    }
    case GetPortStatus: {
        if (wIndex > hcd->num_ports || wIndex < 1) {
            print_error("%s: invalid port number %d\n",
                uhcd->self.bus_name,
                wIndex);
            ret = -EPIPE;
            break;
        }

        /*
         * Whoever resets or resumes must GetPortStatus to
         * complete it !!!
         */

        if (vdphci_port_is_resuming(port) &&
            time_after(jiffies, vdphci_port_get_re_timeout(port))) {
            vdphci_port_set_status_bits(port, USB_PORT_STAT_C_SUSPEND << 16);
            vdphci_port_reset_status_bits(port, USB_PORT_STAT_SUSPEND);
        }

        if (vdphci_port_check_status_bits(port, USB_PORT_STAT_RESET) &&
            time_after(jiffies, vdphci_port_get_re_timeout(port))) {
            vdphci_port_set_status_bits(port, USB_PORT_STAT_C_RESET << 16);
            vdphci_port_reset_status_bits(port, USB_PORT_STAT_RESET);

            if (vdphci_port_is_device_attached(port)) {
                dprintk("%s: port %d enabled\n",
                    uhcd->self.bus_name,
                    port->number);

                vdphci_port_set_status_bits(port, USB_PORT_STAT_ENABLE);
            }
        }

        vdphci_port_update(port, &giveback_list);

        ((u16*)buf)[0] = cpu_to_le16(vdphci_port_get_status(port));
        ((u16*)buf)[1] = cpu_to_le16(vdphci_port_get_status(port) >> 16);

        break;
    }
    case SetHubFeature: {
        ret = -EPIPE;
        break;
    }
    case SetPortFeature: {
        if (wIndex > hcd->num_ports || wIndex < 1) {
            print_error("%s: invalid port number %d\n",
                uhcd->self.bus_name,
                wIndex);
            ret = -EPIPE;
            break;
        }

        switch (wValue) {
        case USB_PORT_FEAT_SUSPEND: {
            if (vdphci_port_is_enabled(port)) {
                vdphci_port_set_status_bits(port, USB_PORT_STAT_SUSPEND);
                vdphci_port_update(port, &giveback_list);
            }
            break;
        }
        case USB_PORT_FEAT_POWER: {
            vdphci_port_set_status_bits(port, USB_PORT_STAT_POWER);
            vdphci_port_update(port, &giveback_list);
            break;
        }
        case USB_PORT_FEAT_RESET: {
            vdphci_port_reset_status_bits(port, (USB_PORT_STAT_ENABLE |
                USB_PORT_STAT_LOW_SPEED |
                USB_PORT_STAT_HIGH_SPEED));
            vdphci_port_set_re_timeout(port, 50);
        }
        default:
            if (vdphci_port_check_status_bits(port, USB_PORT_STAT_POWER)) {
                vdphci_port_set_status_bits(port, 1 << wValue);
                vdphci_port_update(port, &giveback_list);
            }
        }

        break;
    }
    default:
        print_error("%s: no such request\n", uhcd->self.bus_name);
        ret = -EPIPE;
        break;
    }

    if ((wIndex <= hcd->num_ports) && (wIndex >= 1)) {
#ifdef DEBUG
        char* status_str = vdphci_port_status_to_str(vdphci_port_get_status(port));
        if (status_str) {
            dprintk("%s: port %d status: %s\n",
                uhcd->self.bus_name,
                port->number,
                status_str);
            vdphci_port_free_status_str(status_str);
        }
#endif
        need_invalidate = vdphci_port_check_status_bits(port, VDPHCI_PORT_C_MASK);
    }

    spin_unlock_irqrestore(&hcd->lock, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    if (need_invalidate) {
        usb_hcd_poll_rh_status(uhcd);
    }

    dprintk("exit\n");

    return ret;
}

static int vdphci_bus_suspend(struct usb_hcd* uhcd)
{
    unsigned long flags;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);
    u8 port_number;
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    dprintk("enter\n");

    spin_lock_irqsave(&hcd->lock, flags);

    for (port_number = 0; port_number < hcd->num_ports; ++port_number) {
        struct vdphci_port* port = &hcd->ports[port_number];

        vdphci_port_set_hcd_suspended(port, 1);
        vdphci_port_update(port, &giveback_list);
    }

    hcd->suspended = 1;
    uhcd->state = HC_STATE_SUSPENDED;

    spin_unlock_irqrestore(&hcd->lock, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    dprintk("exit\n");

    return 0;
}

static int vdphci_bus_resume(struct usb_hcd* uhcd)
{
    unsigned long flags;
    int res = 0;
    struct vdphci_hcd* hcd = usb_hcd_to_vdphci_hcd(uhcd);
    u8 port_number;
    struct list_head giveback_list;

    INIT_LIST_HEAD(&giveback_list);

    dprintk("enter\n");

    spin_lock_irqsave(&hcd->lock, flags);

    if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &uhcd->flags)) {
        res = -ESHUTDOWN;
    } else {
        hcd->suspended = 0;
        uhcd->state = HC_STATE_RUNNING;

        for (port_number = 0; port_number < hcd->num_ports; ++port_number) {
            struct vdphci_port* port = &hcd->ports[port_number];

            vdphci_port_set_hcd_suspended(port, 0);
            vdphci_port_update(port, &giveback_list);
        }
    }

    spin_unlock_irqrestore(&hcd->lock, flags);

    vdphci_port_giveback_urbs(&giveback_list);

    dprintk("exit\n");

    return res;
}

static const struct hc_driver vdphci_hc_driver =
{
    .description = (char*)vdphci_hcd_name,
    .product_desc = "Virtual Device Platform Host Controller",
    .hcd_priv_size = sizeof(struct vdphci_hcd),
    .flags = HCD_USB2,
    .start = vdphci_start,
    .stop = vdphci_stop,
    .urb_enqueue = vdphci_urb_enqueue,
    .urb_dequeue = vdphci_urb_dequeue,
    .get_frame_number = vdphci_get_frame_number,
    .hub_status_data = vdphci_hub_status_data,
    .hub_control = vdphci_hub_control,
    .bus_suspend = vdphci_bus_suspend,
    .bus_resume = vdphci_bus_resume
};

int vdphci_hcd_add(struct device* controller,
    const char* bus_name,
    const struct vdphci_platform_data* platform_data,
    struct usb_hcd** hcd)
{
    int ret;
    struct vdphci_hcd* vdphcd;

    if (platform_data->num_ports > VDPHCI_MAX_PORTS) {
        print_error("%s: num_ports must be <= %d\n", bus_name, VDPHCI_MAX_PORTS);
        return -EINVAL;
    }

    *hcd = usb_create_hcd(&vdphci_hc_driver, controller, bus_name);

    if (!*hcd) {
        return -ENOMEM;
    }

    (*hcd)->has_tt = 1;

    /*
     * fill HCD platform data
     */

    vdphcd = usb_hcd_to_vdphci_hcd(*hcd);

    vdphcd->num_ports = platform_data->num_ports;
    vdphcd->major = platform_data->major;

    /*
     * add HCD to system
     */

    ret = usb_add_hcd(*hcd, 0, 0);

    if (ret != 0) {
        usb_put_hcd(*hcd);
        *hcd = 0;
    }

    return ret;
}

void vdphci_hcd_remove(struct usb_hcd* hcd)
{
    usb_remove_hcd(hcd);
    usb_put_hcd(hcd);
}

void vdphci_hcd_invalidate_ports(struct vdphci_hcd* hcd)
{
    struct usb_hcd* uhcd = vdphci_hcd_to_usb_hcd(hcd);

    usb_hcd_poll_rh_status(uhcd);
}
