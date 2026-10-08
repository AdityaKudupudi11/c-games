#ifndef SIM_H
#define SIM_H


#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#define MAX_TAIL          1024
#define MAX_HISTORY       4096
#define MAX_COLORS        2

#define REGION_ROWS       10
#define REGION_COLS       20
#define REGION_COUNT      (REGION_ROWS * REGION_COLS)

#define SEGMENT_DIST      1.5f     // distance along the path between tail segments
#define HISTORY_PREFILL   40       // path points laid out behind the head at spawn
#define START_TAIL        7
#define START_LIVES       3
#define SAFE_TAIL_SEGMENTS 5       // the first N segments can never hit the head
#define BASE_SPEED        30.f
#define MAX_SPEED         90.f     // keeps per-tick movement smaller than the head
#define FOOD_PULSE_SPEED  6.f      // world units / second
#define GAME_OVER_TIME    1.5f     // seconds the "GAME OVER" banner stays up
#define DIR_QUEUE_MAX     3

// 0 = leaving the arena wraps to the opposite side (what the old comment said)
// 1 = touching a wall ends the round (what the old code actually did)
#ifndef WALLS_KILL
#define WALLS_KILL 0
#endif

typedef struct {
    v2 min;
    v2 max;
    b32 occupied;
} Region;

typedef struct {
    //snake head attributes
    v2 Snake_head_p;
    v2 Snake_head_dir;        // unit axis vector; velocity is ALWAYS dir * speed
    v2 Snake_head_last_rec;   // last point written into path_history
    v2 Snake_head_half_size;
    u32 Snake_head_color;
    f32 Snake_head_speed;

    // food attributes
    v2 food_p;
    v2 food_half_size;
    v2 food_half_size_range;   // x = min, y = max
    f32 food_pulse_dir;        // +1 growing, -1 shrinking
    u32 food_color;

    // arena attributes
    v2 arena_half_size;
    Region regions[REGION_COUNT];
    f32 region_w, region_h;

    //tail and path history
    v2 tail[MAX_TAIL];
    v2 path_history[MAX_HISTORY];
    int history_count;

    int tail_count,lives ,score, high_score , food_eaten,food_color_idx;
    f32 game_over_timer ;
    v2 dir_queue[DIR_QUEUE_MAX];
    int dir_queue_count ;
    // b32 game_initialized;
} sim;

u32 colors[MAX_COLORS] = {0x45FFB7, 0xD445FF};
#endif
