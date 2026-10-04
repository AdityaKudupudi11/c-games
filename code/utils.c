#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;

typedef int16_t s16;
typedef uint16_t u16;

typedef int32_t s32;
typedef uint32_t u32;

typedef int64_t s64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef int b32;

#define true 1
#define false 0

#define global static
#define internal static

#define ArrayCount(a) ((int)(sizeof(a) / sizeof((a)[0])))

global b32 running = true;

typedef struct {
    f32 x;
    f32 y;
} v2;
