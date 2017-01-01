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

vdp_u32 vdp_usb_write_string_descriptor(const struct vdp_usb_string_table* tables,
    vdp_u16 language_id,
    vdp_u8 index,
    vdp_byte* buff,
    vdp_u32 buff_size)
{
    vdp_u32 tmp = 0;
    vdp_u32 num_written = 0;
    const struct vdp_usb_string_table* table = NULL;
    struct vdp_usb_string* string = NULL;

    assert(tables);
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
    const struct vdp_usb_descriptor_header** other,
    vdp_byte* buff,
    vdp_u32 buff_size)
{
    vdp_u32 tmp;
    const struct vdp_usb_descriptor_header** iter;
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
