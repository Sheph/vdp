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

#include "vdp_usb_context.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

static lwl_priority_t vdp_usb_translate_log_level(vdp_log_level log_level)
{
    switch (log_level) {
    case vdp_log_critical: return LWL_PRI_CRIT;
    case vdp_log_error: return LWL_PRI_ERR;
    case vdp_log_warning: return LWL_PRI_WARNING;
    case vdp_log_info: return LWL_PRI_INFO;
    case vdp_log_debug: return LWL_PRI_DEBUG;
    default: return LWL_PRI_DEBUG;
    }
}

vdp_usb_result vdp_usb_init(FILE* log_file,
    vdp_log_level log_level,
    struct vdp_usb_context** context)
{
    vdp_usb_result vdp_res = vdp_usb_unknown;

    assert(context);
    if (!context) {
        return vdp_usb_misuse;
    }

    *context = malloc(sizeof(**context));

    if (*context == NULL) {
        return vdp_usb_nomem;
    }

    memset(*context, 0, sizeof(**context));

    if (log_file != NULL) {
        (*context)->logger = lwl_alloc();

        if ((*context)->logger == NULL) {
            vdp_res = vdp_usb_nomem;

            goto fail;
        }

        if (lwl_set_attributes((*context)->logger,
            LWL_TAG_PREFIX, "vdp_usb",
            LWL_TAG_OPTIONS, LWL_OPT_DATE | LWL_OPT_PREFIX | LWL_OPT_PRIORITY | LWL_OPT_TIME | LWL_OPT_USE_LOCALE,
            LWL_TAG_FILE, log_file,
            LWL_TAG_LEVEL, vdp_usb_translate_log_level(log_level),
            LWL_TAG_DONE ) != 0) {
            vdp_res = vdp_usb_unknown;

            goto fail;
        }
    }

    VDP_USB_LOG_DEBUG(*context, "context initialized");

    return vdp_usb_success;

fail:
    if ((*context)->logger) {
        lwl_free((*context)->logger);
    }

    free(*context);
    *context = NULL;

    return vdp_res;
}

void vdp_usb_cleanup(struct vdp_usb_context* context)
{
    assert(context);
    if (!context) {
        return;
    }

    VDP_USB_LOG_DEBUG(context, "cleaning up context");

    if (context->logger) {
        lwl_free(context->logger);
    }

    free(context);
}

vdp_usb_result vdp_usb_get_device_range(struct vdp_usb_context* context, vdp_u8* device_lower, vdp_u8* device_upper)
{
    DIR* dir;
    struct dirent* de;
    int found = 0;

    assert(context);
    if (!context) {
        return vdp_usb_misuse;
    }

    dir = opendir("/dev");

    if (dir == NULL) {
        int error = errno;

        VDP_USB_LOG_ERROR(context, "cannot open /dev: %s (%d)", strerror(error), error);

        return vdp_usb_not_found;
    }

    if (device_lower) {
        *device_lower = 0;
    }

    if (device_upper) {
        *device_upper = 0;
    }

    errno = 0;

    while ((de = readdir(dir))) {
        const char* f_name = &de->d_name[0];

        if ((de->d_type == DT_CHR) &&
            (strncmp(f_name, VDPHCI_DEVICE_PREFIX, strlen(VDPHCI_DEVICE_PREFIX)) == 0)) {
            int device_num;

            int ret = sscanf(f_name + strlen(VDPHCI_DEVICE_PREFIX), "%d", &device_num);

            if ((ret == EOF) || (ret == 0)) {
                VDP_USB_LOG_WARNING(context, "device \"%s\" does not contain a number", f_name);

                continue;
            }

            if (device_num < 0) {
                VDP_USB_LOG_WARNING(context, "device \"%s\" number is less than zero", f_name);

                continue;
            }

            found = 1;

            if (device_lower && device_num < *device_lower) {
                *device_lower = device_num;
            }

            if (device_upper && device_num > *device_upper) {
                *device_upper = device_num;
            }
        }
    }

    closedir(dir);

    return (found ? vdp_usb_success : vdp_usb_not_found);
}
