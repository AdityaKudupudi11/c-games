// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <time.h>

// ---- Basic typedefs ----
typedef int8_t   s8;
typedef uint8_t  u8;

typedef int16_t  s16;
typedef uint16_t u16;

typedef int32_t  s32;
typedef uint32_t u32;

typedef int64_t  s64;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef int      b32;

// ---- Boolean macros ----
#define true  1
#define false 0

// ---- Linkage ----
#define global   static
#define internal static

// ---- Global state ----
global b32 running;

// ---- Structs ----
typedef struct {
    f32 x;
    f32 y;
} v2;

typedef struct {
    b32 is_down;
    b32 changed;
    b32 was_pressed;
} Button;

enum {
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_COUNT,
};

typedef struct {
    Button buttons[BUTTON_COUNT];
} Input;

#define pressed(b)  (input->buttons[b].was_pressed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)
#define is_down(b)  (input->buttons[b].is_down)

// ---- Function prototypes (from math.c) ----
void init_seed();
f32  clamp(f32 min, f32 val, f32 max);
u32  random_color();
f32  random_num_generator();
f32  random_num_generator_in_range(f32 min, f32 max);

v2   add_v2(v2 a, v2 b);
v2   sub_v2(v2 a, v2 b);
v2   add_scalar_v2(v2 a, f32 p);
v2   clamp_v2(v2 a, v2 b, v2 c);

b32 is_colliding(v2 p1,v2 p2, v2 half_size1 , v2 half_size2);
#define ArrayCount(a) ((int)(sizeof(a) / sizeof((a)[0])))

#endif // COMMON_H
