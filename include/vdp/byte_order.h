#ifndef _VDP_BYTE_ORDER_H_
#define _VDP_BYTE_ORDER_H_

#include "vdp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

static __inline vdp_u16 vdp_swap_u16(vdp_u16 value)
{
    return ((value >> 8) & 0x00FF) | ((value << 8) & 0xFF00);
}

static __inline vdp_s16 vdp_swap_s16(vdp_s16 value)
{
    return (vdp_s16)vdp_swap_u16((vdp_u16)value);
}

static __inline vdp_u32 vdp_swap_u32(vdp_u32 value)
{
    return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) |
           ((value << 8) & 0x00FF0000) | ((value << 24) & 0xFF000000);
}

static __inline vdp_s32 vdp_swap_s32(vdp_s32 value)
{
    return (vdp_s32)vdp_swap_u32((vdp_u32)value);
}

static __inline vdp_u64 vdp_swap_u64(vdp_u64 value)
{
    vdp_u32 hi = (vdp_u32)(value >> 32);
    vdp_u32 lo = (vdp_u32)(value & 0xFFFFFFFF);
    return (vdp_u64)vdp_swap_u32(hi) | ((vdp_u64)vdp_swap_u32(lo) << 32);
}

static __inline vdp_s64 vdp_swap_s64(vdp_s64 value)
{
    return (vdp_s64)vdp_swap_u64((vdp_u64)value);
}

#define VDP_DEFINE_CPUTO_NOOP(type, endian) \
    static __inline vdp_##type vdp_cpu_to_##type##endian(vdp_##type value) \
    { \
        return value; \
    }

#define VDP_DEFINE_CPUTO_SWAP(type, endian) \
    static __inline vdp_##type vdp_cpu_to_##type##endian(vdp_##type value) \
    { \
        return vdp_swap_##type(value); \
    }

#define VDP_DEFINE_TOCPU_NOOP(type, endian) \
    static __inline vdp_##type vdp_##type##endian##_to_cpu(vdp_##type value) \
    { \
        return value; \
    }

#define VDP_DEFINE_TOCPU_SWAP(type, endian) \
    static __inline vdp_##type vdp_##type##endian##_to_cpu(vdp_##type value) \
    { \
        return vdp_swap_##type(value); \
    }

#ifdef VDP_LITTLE_ENDIAN
#define VDP_DEFINE_CPUTO_LE(type) VDP_DEFINE_CPUTO_NOOP(type, le)
#define VDP_DEFINE_CPUTO_BE(type) VDP_DEFINE_CPUTO_SWAP(type, be)
#define VDP_DEFINE_TOCPU_LE(type) VDP_DEFINE_TOCPU_NOOP(type, le)
#define VDP_DEFINE_TOCPU_BE(type) VDP_DEFINE_TOCPU_SWAP(type, be)
#else
#define VDP_DEFINE_CPUTO_LE(type) VDP_DEFINE_CPUTO_SWAP(type, le)
#define VDP_DEFINE_CPUTO_BE(type) VDP_DEFINE_CPUTO_NOOP(type, be)
#define VDP_DEFINE_TOCPU_LE(type) VDP_DEFINE_TOCPU_SWAP(type, le)
#define VDP_DEFINE_TOCPU_BE(type) VDP_DEFINE_TOCPU_NOOP(type, be)
#endif

VDP_DEFINE_CPUTO_LE(u16)
VDP_DEFINE_CPUTO_LE(s16)
VDP_DEFINE_CPUTO_LE(u32)
VDP_DEFINE_CPUTO_LE(s32)
VDP_DEFINE_CPUTO_LE(u64)
VDP_DEFINE_CPUTO_LE(s64)
VDP_DEFINE_CPUTO_BE(u16)
VDP_DEFINE_CPUTO_BE(s16)
VDP_DEFINE_CPUTO_BE(u32)
VDP_DEFINE_CPUTO_BE(s32)
VDP_DEFINE_CPUTO_BE(u64)
VDP_DEFINE_CPUTO_BE(s64)
VDP_DEFINE_TOCPU_LE(u16)
VDP_DEFINE_TOCPU_LE(s16)
VDP_DEFINE_TOCPU_LE(u32)
VDP_DEFINE_TOCPU_LE(s32)
VDP_DEFINE_TOCPU_LE(u64)
VDP_DEFINE_TOCPU_LE(s64)
VDP_DEFINE_TOCPU_BE(u16)
VDP_DEFINE_TOCPU_BE(s16)
VDP_DEFINE_TOCPU_BE(u32)
VDP_DEFINE_TOCPU_BE(s32)
VDP_DEFINE_TOCPU_BE(u64)
VDP_DEFINE_TOCPU_BE(s64)

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
