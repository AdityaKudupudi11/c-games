// ---- RNG (xorshift32). One generator for everything, so runs are reproducible
// ---- if you call seed_rng() with a fixed value.
global u32 rng_state = 2463534242u;

internal void
seed_rng(u32 s) {
    rng_state = s ? s : 2463534242u;      // xorshift must never be seeded with 0
}

internal void
init_seed(void) {
    seed_rng((u32)time(0) ^ 0x9E3779B9u);
}

internal u32
random_u32(void) {
    u32 x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

// Uniform in [0, 1). Uses only 24 bits so the f32 conversion is exact and can
// never round up to 1.0f.
internal f32
random_num_generator(void) {
    return (f32)(random_u32() >> 8) * (1.0f / 16777216.0f);
}

internal f32
random_num_generator_in_range(f32 min, f32 max) {
    return min + random_num_generator() * (max - min);
}

// Uniform integer in [0, n).
internal int
random_int(int n) {
    if (n <= 0) return 0;
    return (int)(random_u32() % (u32)n);
}

internal f32
clamp(f32 min, f32 val, f32 max) {
    if (val > max) val = max;
    else if (val < min) val = min;
    return val;
}

internal int
clamp_int(int min, int val, int max) {
    if (val > max) val = max;
    else if (val < min) val = min;
    return val;
}

// Bright-ish colour so the snake never disappears into the dark arena.
internal u32
random_color(void) {
    u32 r = 64 + (u32)random_int(192);
    u32 g = 64 + (u32)random_int(192);
    u32 b = 64 + (u32)random_int(192);
    return (r << 16) | (g << 8) | b;
}

internal v2 add_v2(v2 a, v2 b)            { return (v2){a.x + b.x, a.y + b.y}; }
internal v2 sub_v2(v2 a, v2 b)            { return (v2){a.x - b.x, a.y - b.y}; }
internal v2 scale_v2(v2 a, f32 s)         { return (v2){a.x * s, a.y * s}; }
internal v2 add_scalar_v2(v2 a, f32 p)    { return (v2){a.x + p, a.y + p}; }

// Component-wise clamp of v between lo and hi.
internal v2
clamp_v2(v2 v, v2 lo, v2 hi) {
    return (v2){clamp(lo.x, v.x, hi.x), clamp(lo.y, v.y, hi.y)};
}
