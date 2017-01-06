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

#include "vdp/usb_util.h"
#include "vdp/byte_order.h"
#include <string.h>
#include <assert.h>

static int utf8_to_utf16le(const char* s, vdp_u16* cp, unsigned int len)
{
    int count = 0;
    vdp_u8  c;
    vdp_u16 uchar;
    vdp_u16 tmp;

    /* this insists on correct encodings, though not minimal ones.
     * BUT it currently rejects legit 4-byte UTF-8 code points,
     * which need surrogate pairs.  (Unicode 3.1 can use them.)
     */
    while (len != 0 && (c = (vdp_u8) *s++) != 0) {
        if (c & 0x80) {
            // 2-byte sequence:
            // 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
            if ((c & 0xe0) == 0xc0) {
                uchar = (c & 0x1f) << 6;

                c = (vdp_u8) *s++;
                if ((c & 0xc0) != 0x80)
                    goto fail;
                c &= 0x3f;
                uchar |= c;

            // 3-byte sequence (most CJKV characters):
            // zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
            } else if ((c & 0xf0) == 0xe0) {
                uchar = (c & 0x0f) << 12;

                c = (vdp_u8) *s++;
                if ((c & 0xc0) != 0x80)
                    goto fail;
                c &= 0x3f;
                uchar |= c << 6;

                c = (vdp_u8) *s++;
                if ((c & 0xc0) != 0x80)
                    goto fail;
                c &= 0x3f;
                uchar |= c;

                /* no bogus surrogates */
                if (0xd800 <= uchar && uchar <= 0xdfff)
                    goto fail;

            // 4-byte sequence (surrogate pairs, currently rare):
            // 11101110wwwwzzzzyy + 110111yyyyxxxxxx
            //     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
            // (uuuuu = wwww + 1)
            // FIXME accept the surrogate code points (only)

            } else
                goto fail;
        } else
            uchar = c;
        tmp = vdp_cpu_to_u16le(uchar);
        memcpy(cp++, &tmp, sizeof(tmp));
        count++;
        len--;
    }
    return count;
fail:
    return -1;
}

struct utf8_table
{
    int cmask;
    int cval;
    int shift;
    long lmask;
    long lval;
};

static const struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
    {0,                            /* end of table    */}
};

#define UNICODE_MAX 0x0010ffff
#define PLANE_SIZE 0x00010000
#define SURROGATE_MASK 0xfffff800
#define SURROGATE_PAIR 0x0000d800
#define SURROGATE_LOW 0x00000400
#define SURROGATE_BITS 0x000003ff

static int utf32_to_utf8(unsigned long l, vdp_u8* s, int maxout)
{
    int c, nc;
    const struct utf8_table *t;

    if (!s)
        return 0;

    if (l > UNICODE_MAX || (l & SURROGATE_MASK) == SURROGATE_PAIR)
        return -1;

    nc = 0;
    for (t = utf8_table; t->cmask && maxout; t++, maxout--) {
        nc++;
        if (l <= t->lmask) {
            c = t->shift;
            *s = (vdp_u8)(t->cval | (l >> c));
            while (c > 0) {
                c -= 6;
                s++;
                *s = (vdp_u8)(0x80 | ((l >> c) & 0x3F));
            }
            return nc;
        }
    }

    return -1;
}

int vdp_usb_utf16le_to_utf8(const vdp_u16* pwcs, char* s, unsigned int len)
{
    vdp_u8* op;
    int size;
    unsigned long u, v;
    int inlen = len;
    int maxout = inlen * 3;

    op = (vdp_u8*)s;
    while (inlen > 0 && maxout > 0) {
        u = vdp_u16le_to_cpu(*pwcs);
        if (!u)
            break;
        pwcs++;
        inlen--;
        if (u > 0x7f) {
            if ((u & SURROGATE_MASK) == SURROGATE_PAIR) {
                if (u & SURROGATE_LOW) {
                    /* Ignore character and move on */
                    continue;
                }
                if (inlen <= 0)
                    break;
                v = vdp_u16le_to_cpu(*pwcs);
                if ((v & SURROGATE_MASK) != SURROGATE_PAIR ||
                        !(v & SURROGATE_LOW)) {
                    /* Ignore character and move on */
                    continue;
                }
                u = PLANE_SIZE + ((u & SURROGATE_BITS) << 10)
                        + (v & SURROGATE_BITS);
                pwcs++;
                inlen--;
            }
            size = utf32_to_utf8(u, op, maxout);
            if (size == -1) {
                /* Ignore character and move on */
            } else {
                op += size;
                maxout -= size;
            }
        } else {
            *op++ = (vdp_u8)u;
            maxout--;
        }
    }
    return op - (vdp_u8*)s;
}

vdp_u32 vdp_usb_write_string_descriptor(const struct vdp_usb_string_table* tables,
    vdp_u16 language_id,
    vdp_u8 index,
    vdp_byte* buff,
    vdp_u32 buff_size)
{
    vdp_u32 tmp = 0;
    vdp_u32 num_written = 0;
    const struct vdp_usb_string_table* table = NULL;
    const struct vdp_usb_string* string = NULL;

    if (!tables) {
        return 0;
    }

    if (index == 0) {
        /*
         * Write the language id descriptor
         */

        vdp_u32 count = 0;

        for (table = tables; table->strings != NULL; ++table) {
            ++count;
        }

        struct vdp_usb_string_descriptor descriptor;

        descriptor.bLength = vdp_offsetof(struct vdp_usb_string_descriptor, wData) +
            count * 2;
        descriptor.bDescriptorType = VDP_USB_DT_STRING;

        tmp = vdp_min(vdp_offsetof(struct vdp_usb_string_descriptor, wData), buff_size);

        memcpy(buff, &descriptor, tmp);

        num_written += tmp;
        buff_size -= tmp;

        for (table = tables; (table->strings != NULL) && (buff_size > 0); ++table) {
            vdp_u16 langid = vdp_cpu_to_u16le(table->language_id);

            tmp = vdp_min(2, buff_size);

            memcpy(buff + num_written, &langid, tmp);

            num_written += tmp;
            buff_size -= tmp;
        }

        return num_written;
    }

    for (table = tables; table->strings != NULL; ++table) {
        if (table->language_id != language_id) {
            continue;
        }

        assert(table->strings);
        if (!table->strings) {
            continue;
        }

        for (string = table->strings; string->str != NULL; ++string) {
            vdp_u32 str_len;
            struct vdp_usb_string_descriptor descriptor;

            if (string->index != index) {
                continue;
            }

            str_len = strlen(string->str);

            descriptor.bLength = vdp_offsetof(struct vdp_usb_string_descriptor, wData) +
                (str_len * 2);
            descriptor.bDescriptorType = VDP_USB_DT_STRING;

            tmp = vdp_min(vdp_offsetof(struct vdp_usb_string_descriptor, wData), buff_size);

            memcpy(buff, &descriptor, tmp);

            num_written += tmp;
            buff_size -= tmp;

            tmp = vdp_min(str_len * 2, buff_size);

            memset(buff + num_written, 0, tmp);

            if (utf8_to_utf16le(string->str, (vdp_u16*)(buff + num_written), (tmp / 2)) < 0) {
                return 0;
            }

            return num_written + tmp;
        }
    }

    return 0;
}

vdp_u32 vdp_usb_write_device_descriptor(
    const struct vdp_usb_device_descriptor* descriptor,
    vdp_byte* buff,
    vdp_u32 buff_size)
{
    vdp_u32 tmp;

    assert(descriptor);
    if (!descriptor) {
        return 0;
    }

    assert(descriptor->bLength <= sizeof(*descriptor));
    if (descriptor->bLength > sizeof(*descriptor)) {
        return 0;
    }

    tmp = vdp_min(descriptor->bLength, buff_size);

    memcpy(buff, descriptor, tmp);

    return tmp;
}

vdp_u32 vdp_usb_write_config_descriptor(
    const struct vdp_usb_config_descriptor* descriptor,
    struct vdp_usb_descriptor_header** other,
    vdp_byte* buff,
    vdp_u32 buff_size)
{
    vdp_u32 tmp;
    struct vdp_usb_descriptor_header** iter;
    vdp_u32 total;
    struct vdp_usb_config_descriptor tmp_descriptor;

    assert(descriptor);

    if (!descriptor) {
        return 0;
    }

    assert(descriptor->bLength <= sizeof(*descriptor));
    if (descriptor->bLength > sizeof(*descriptor)) {
        return 0;
    }

    memcpy(&tmp_descriptor, descriptor, sizeof(tmp_descriptor));

    total = tmp_descriptor.bLength;

    for (iter = other; iter && (*iter != NULL); ++iter) {
        total += (*iter)->bLength;
    }

    tmp_descriptor.wTotalLength = vdp_cpu_to_u16le(total);

    tmp = vdp_min(tmp_descriptor.bLength, buff_size);

    memcpy(buff, &tmp_descriptor, tmp);

    buff += tmp;
    buff_size -= tmp;
    iter = other;
    total = tmp;

    while ((buff_size > 0) && iter && *iter) {
        tmp = vdp_min((*iter)->bLength, buff_size);

        memcpy(buff, *iter, tmp);

        buff += tmp;
        buff_size -= tmp;
        ++iter;
        total += tmp;
    }

    return total;
}

vdp_u32 vdp_usb_write_qualifier_descriptor(
    const struct vdp_usb_qualifier_descriptor* descriptor,
    vdp_byte* buff,
    vdp_u32 buff_size)
{
    vdp_u32 tmp;

    assert(descriptor);
    if (!descriptor) {
        return 0;
    }

    assert(descriptor->bLength <= sizeof(*descriptor));
    if (descriptor->bLength > sizeof(*descriptor)) {
        return 0;
    }

    tmp = vdp_min(descriptor->bLength, buff_size);

    memcpy(buff, descriptor, tmp);

    return tmp;
}
