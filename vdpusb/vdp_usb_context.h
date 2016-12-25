#ifndef _VDP_USB_CONTEXT_H_
#define _VDP_USB_CONTEXT_H_

#include "vdp/usb.h"
#include "vdphci-common.h"
#include "lwl.h"

struct vdp_usb_context
{
    lwlh_t logger;
};

#define VDP_USB_LOG(context, prio, fmt, ...) \
    do { \
        if ((context)->logger) { \
            lwl_write_log((context)->logger, (prio), "%s: " fmt, __FUNCTION__,##__VA_ARGS__); \
        } \
    } while (0)

#define VDP_USB_LOG_CRITICAL(context, fmt, ...) VDP_USB_LOG(context, LWL_PRI_CRIT, fmt,##__VA_ARGS__)
#define VDP_USB_LOG_ERROR(context, fmt, ...) VDP_USB_LOG(context, LWL_PRI_ERR, fmt,##__VA_ARGS__)
#define VDP_USB_LOG_WARNING(context, fmt, ...) VDP_USB_LOG(context, LWL_PRI_WARNING, fmt,##__VA_ARGS__)
#define VDP_USB_LOG_INFO(context, fmt, ...) VDP_USB_LOG(context, LWL_PRI_INFO, fmt,##__VA_ARGS__)
#define VDP_USB_LOG_DEBUG(context, fmt, ...) VDP_USB_LOG(context, LWL_PRI_DEBUG, fmt,##__VA_ARGS__)

#define VDP_USB_LOG_ENABLED(context, prio) \
    ((context)->logger && (lwl_get_log_level((context)->logger) <= (prio)))

#define VDP_USB_LOG_CRITICAL_ENABLED(context) VDP_USB_LOG_ENABLED(context, LWL_PRI_CRIT)
#define VDP_USB_LOG_ERROR_ENABLED(context) VDP_USB_LOG_ENABLED(context, LWL_PRI_ERR)
#define VDP_USB_LOG_WARNING_ENABLED(context) VDP_USB_LOG_ENABLED(context, LWL_PRI_WARNING)
#define VDP_USB_LOG_INFO_ENABLED(context) VDP_USB_LOG_ENABLED(context, LWL_PRI_INFO)
#define VDP_USB_LOG_DEBUG_ENABLED(context) VDP_USB_LOG_ENABLED(context, LWL_PRI_DEBUG)

#endif
