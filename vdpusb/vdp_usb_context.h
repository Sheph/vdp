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
