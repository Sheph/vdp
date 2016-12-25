#ifndef _VDP_TYPES_H_
#define _VDP_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__i386) || defined(_M_IX86)
#define VDP_LITTLE_ENDIAN
#elif defined(__x86_64) || defined(_M_X64) || defined(_M_IA64)
#define VDP_LITTLE_ENDIAN
#else
#error "architecture not supported"
#endif

typedef int8_t vdp_s8;
typedef uint8_t vdp_u8;
typedef int16_t vdp_s16;
typedef uint16_t vdp_u16;
typedef int32_t vdp_s32;
typedef uint32_t vdp_u32;
typedef int64_t vdp_s64;
typedef uint64_t vdp_u64;

typedef vdp_u8 vdp_byte;

#ifdef linux
typedef int vdp_fd;
#else
#error "platform not supported"
#endif

#define vdp_offsetof(type, member) ((size_t)&((type*)0)->member)

#define vdp_containerof(ptr, type, member) ((type*)((char*)(ptr) - vdp_offsetof(type, member)))

#define vdp_min(a, b) (((a)<(b))?(a):(b))
#define vdp_max(a, b) (((a)>(b))?(a):(b))

/*
 * Log level.
 */

typedef enum
{
    vdp_log_critical = 0,
    vdp_log_error = 1,
    vdp_log_warning = 2,
    vdp_log_info = 3,
    vdp_log_debug = 4
} vdp_log_level;

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
