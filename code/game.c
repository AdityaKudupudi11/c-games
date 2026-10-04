/*
TODO (from your original list):
-file saving system (high score)
-basic background music
-later try for new levels
-particle system for collision explosion
-add few powerups
*/

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
    v2 p;
    v2 dir;        // unit axis vector; velocity is ALWAYS dir * speed
    v2 last_rec;   // last point written into path_history
    v2 half_size;
    u32 color;
    f32 speed;
} Snake_head;

typedef struct {
    v2 p;
} Body;

typedef struct {
    v2 p;
    v2 half_size;
    v2 half_size_range;   // x = min, y = max
    f32 pulse_dir;        // +1 growing, -1 shrinking
    u32 color;
} Food;

// Flat snapshot for the agent (kept for later, not used by the game itself).
typedef struct {
    f32 head_x, head_y;
    f32 dir_x, dir_y;
    f32 food_x, food_y;
    int tail_count;
    float tail_positions[MAX_TAIL * 2];
    int lives;
    f32 speed;
    int food_eaten;
    int score;
} GameState;

global u32 colors[MAX_COLORS] = {0x45FFB7, 0xD445FF};
global int color_idx = 0;

global v2 arena_half_size;
global Region regions[REGION_COUNT];
global f32 region_w, region_h;

global Body tail[MAX_TAIL];
global Food food;
global Snake_head head;

global v2 path_history[MAX_HISTORY];
global int history_count = 0;

global int tail_count = START_TAIL;
global int lives = START_LIVES;
global int score = 0;
global int high_score = 0;
global int food_eaten = 0;

global f32 game_over_timer = 0.f;
global v2 dir_queue[DIR_QUEUE_MAX];
global int dir_queue_count = 0;
global b32 game_initialized = false;


// ------------------------------------------------------------ helpers

internal void
push_history(v2 p) {
    path_history[history_count % MAX_HISTORY] = p;
    history_count++;
}

// Tail segment i sits on the i-th most recent path point.
internal void
update_tail(void) {
    for (int i = 0; i < tail_count; i++) {
        int idx = history_count - (i + 1);
        if (idx >= 0) tail[i].p = path_history[idx % MAX_HISTORY];
    }
}

internal int
get_region_index(v2 p) {
    f32 margin_x = arena_half_size.x - food.half_size_range.y;
    f32 margin_y = arena_half_size.y - food.half_size_range.y;

    int col = (int)floorf((p.x + margin_x) / region_w);
    int row = (int)floorf((p.y + margin_y) / region_h);

    col = clamp_int(0, col, REGION_COLS - 1);
    row = clamp_int(0, row, REGION_ROWS - 1);
    return row * REGION_COLS + col;
}

internal void
update_regions(void) {
    for (int i = 0; i < REGION_COUNT; i++) regions[i].occupied = false;

    for (int i = 0; i < tail_count; i++) {
        regions[get_region_index(tail[i].p)].occupied = true;
    }
    regions[get_region_index(head.p)].occupied = true;
}

internal void
food_spawn(void) {
    update_regions();

    // Choose uniformly among FREE regions. The old do/while could never pick
    // region 199 and looped forever once every region was occupied.
    int free_ids[REGION_COUNT];
    int free_count = 0;
    for (int i = 0; i < REGION_COUNT; i++) {
        if (!regions[i].occupied) free_ids[free_count++] = i;
    }
    int id = free_count ? free_ids[random_int(free_count)] : random_int(REGION_COUNT);

    food.p.x = random_num_generator_in_range(regions[id].min.x, regions[id].max.x);
    food.p.y = random_num_generator_in_range(regions[id].min.y, regions[id].max.y);
}


// ------------------------------------------------------------ state setup

// Starts a fresh round. high_score is intentionally NOT touched.
internal void
reset_round(void) {
    head.p = (v2){-20.f, 0.f};
    head.dir = (v2){1.f, 0.f};
    head.last_rec = head.p;
    head.speed = BASE_SPEED;
    head.color = 0x50E66A;

    tail_count = START_TAIL;
    lives = START_LIVES;
    score = 0;
    food_eaten = 0;
    game_over_timer = 0.f;
    dir_queue_count = 0;

    // Lay a straight trail behind the head so the whole tail exists (and is
    // visible, and counts for food placement) from the very first frame.
    history_count = 0;
    for (int k = HISTORY_PREFILL; k >= 1; k--) {
        push_history((v2){head.p.x - k * SEGMENT_DIST, head.p.y});
    }
    update_tail();

    food.half_size = (v2){food.half_size_range.y, food.half_size_range.y};
    food.pulse_dir = -1.f;
    food_spawn();
}

internal void
init_game(void) {
    init_seed();
    memset(&head, 0, sizeof(head));
    memset(&food, 0, sizeof(food));
    memset(tail, 0, sizeof(tail));
    memset(path_history, 0, sizeof(path_history));

    head.half_size = (v2){1.5f, 1.5f};

    food.half_size_range = (v2){0.75f, 2.5f};
    food.color = colors[0];

    arena_half_size = (v2){85.f, 40.f};
    region_w = ((arena_half_size.x - food.half_size_range.y) * 2) / REGION_COLS;
    region_h = ((arena_half_size.y - food.half_size_range.y) * 2) / REGION_ROWS;

    int idx = 0;
    for (int r = 0; r < REGION_ROWS; r++) {
        for (int c = 0; c < REGION_COLS; c++) {
            regions[idx].min = (v2){-(arena_half_size.x - food.half_size_range.y) + c * region_w,
                                    -(arena_half_size.y - food.half_size_range.y) + r * region_h};
            regions[idx].max = (v2){regions[idx].min.x + region_w, regions[idx].min.y + region_h};
            regions[idx].occupied = false;
            idx++;
        }
    }

    game_initialized = true;
    reset_round();
}

internal void
begin_game_over(void) {
    game_over_timer = GAME_OVER_TIME;
}


// ------------------------------------------------------------ input

// Queue a turn, validated against the heading it will follow (the last queued
// turn, or the current heading). Only 90-degree turns are accepted, so the
// snake can never reverse into itself, no matter how fast keys are mashed.
internal void
queue_direction(v2 d) {
    if (dir_queue_count >= DIR_QUEUE_MAX) return;
    v2 ref = dir_queue_count ? dir_queue[dir_queue_count - 1] : head.dir;
    f32 dot = ref.x * d.x + ref.y * d.y;
    if (fabsf(dot) < 0.5f) dir_queue[dir_queue_count++] = d;
}


// ------------------------------------------------------------ simulation

internal void
update_game(Input *input, f32 dt) {
    if (!game_initialized) init_game();

    if (game_over_timer > 0.f) {
        game_over_timer -= dt;
        if (game_over_timer <= 0.f) reset_round();
        return;
    }

    // --- turning: one queued turn is applied per tick ---
    if (pressed(BUTTON_UP))    queue_direction((v2){0.f, 1.f});
    if (pressed(BUTTON_DOWN))  queue_direction((v2){0.f, -1.f});
    if (pressed(BUTTON_LEFT))  queue_direction((v2){-1.f, 0.f});
    if (pressed(BUTTON_RIGHT)) queue_direction((v2){1.f, 0.f});
    if (dir_queue_count > 0) {
        head.dir = dir_queue[0];
        for (int i = 1; i < dir_queue_count; i++) dir_queue[i - 1] = dir_queue[i];
        dir_queue_count--;
    }

    // --- move head. Velocity comes from dir*speed every tick, so speed-ups
    //     from eating take effect immediately (before, dp was a stale copy). ---
    head.p.x += head.dir.x * head.speed * dt;
    head.p.y += head.dir.y * head.speed * dt;

    // --- arena walls (the head stays fully inside the play area) ---
    f32 lim_x = arena_half_size.x - head.half_size.x;
    f32 lim_y = arena_half_size.y - head.half_size.y;
#if WALLS_KILL
    if (head.p.x > lim_x || head.p.x < -lim_x || head.p.y > lim_y || head.p.y < -lim_y) {
        begin_game_over();
        return;
    }
#else
    b32 wrapped = false;
    if (head.p.x > lim_x)       { head.p.x -= 2.f * lim_x; wrapped = true; }
    else if (head.p.x < -lim_x) { head.p.x += 2.f * lim_x; wrapped = true; }
    if (head.p.y > lim_y)       { head.p.y -= 2.f * lim_y; wrapped = true; }
    else if (head.p.y < -lim_y) { head.p.y += 2.f * lim_y; wrapped = true; }
    if (wrapped) head.last_rec = head.p;   // don't interpolate across the arena
#endif

    // --- record the path at exact SEGMENT_DIST intervals, so the tail spacing
    //     does not depend on frame rate ---
    for (int guard = 0; guard < 16; guard++) {
        f32 dx = head.p.x - head.last_rec.x;
        f32 dy = head.p.y - head.last_rec.y;
        f32 d2 = dx * dx + dy * dy;
        if (d2 < SEGMENT_DIST * SEGMENT_DIST) break;
        f32 d = sqrtf(d2);
        head.last_rec.x += dx / d * SEGMENT_DIST;
        head.last_rec.y += dy / d * SEGMENT_DIST;
        push_history(head.last_rec);
    }
    update_tail();

    // --- self collision: cut the tail at the hit segment, lose a life ---
    for (int i = SAFE_TAIL_SEGMENTS; i < tail_count; i++) {
        if (is_colliding(head.p, tail[i].p, head.half_size, head.half_size)) {
            tail_count = i;
            head.color = random_color();
            lives--;
            if (lives <= 0) {
                lives = 0;
                begin_game_over();
                return;
            }
            break;
        }
    }

    // --- food pulse (time based, flips colour at each extreme) ---
    f32 size = food.half_size.x + food.pulse_dir * FOOD_PULSE_SPEED * dt;
    if (size >= food.half_size_range.y) {
        size = food.half_size_range.y;
        food.pulse_dir = -1.f;
        color_idx = (color_idx + 1) % MAX_COLORS;
        food.color = colors[color_idx];
    } else if (size <= food.half_size_range.x) {
        size = food.half_size_range.x;
        food.pulse_dir = 1.f;
        color_idx = (color_idx + 1) % MAX_COLORS;
        food.color = colors[color_idx];
    }
    food.half_size = (v2){size, size};

    // --- eating ---
    if (is_colliding(head.p, food.p, head.half_size, food.half_size)) {
        food_eaten++;
        score += tail_count;
        if (score > high_score) high_score = score;
        head.speed = clamp(BASE_SPEED, head.speed + tail_count / 50.f, MAX_SPEED);

        if (tail_count < MAX_TAIL) {
            tail[tail_count].p = tail_count > 0 ? tail[tail_count - 1].p : head.p;
            tail_count++;
        }
        food_spawn();
    }
}


// ------------------------------------------------------------ rendering

internal void
draw_heart(v2 p, u32 color) {
    static const char *heart[] = {
        " ***    ***",
        "*****  *****",
        " **********",
        "  ********",
        "   ******",
        "    ****",
        "     **",
    };

    f32 block_half_size = .15f;
    p.x -= block_half_size * 11;
    f32 original_x = p.x;
    for (int i = 0; i < ArrayCount(heart); i++) {
        for (const char *at = heart[i]; *at; at++) {
            if (*at != ' ') {
                draw(p, (v2){block_half_size, block_half_size}, color, SHAPE_RECT);
            }
            p.x += block_half_size * 2.f;
        }
        p.y -= block_half_size * 2.f;
        p.x = original_x;
    }
}

internal void
render_game(void) {
    clear_screen(0xE69F50);
    draw((v2){0.f, 0.f}, arena_half_size, 0x262D27, SHAPE_RECT);

    // lives
    v2 heart_p = (v2){-arena_half_size.x + 3.f, arena_half_size.y + 3.f};
    for (int i = 0; i < lives; i++) {
        draw_heart(heart_p, 0xEA2323);
        heart_p.x += 5.f;
    }

    // tail (oldest first so newer segments overlap older ones), then head
    for (int i = tail_count - 1; i >= 0; i--) {
        draw(tail[i].p, head.half_size, head.color, SHAPE_CIRCLE);
    }
    draw(head.p, head.half_size, head.color, SHAPE_CIRCLE);

    draw(food.p, food.half_size, food.color, SHAPE_CIRCLE);

    // HUD
    u32 text_color = 0x4A1D54;
    u32 num_color = 0xFEF6EB;
    draw_words((v2){-arena_half_size.x + 4.f, arena_half_size.y + 6.f}, text_color, "LIVES");
    draw_words((v2){-8.f, arena_half_size.y + 6.f}, text_color, "HIGH SCORE");
    draw_score((v2){0.f, arena_half_size.y + 3.f}, num_color, (u32)high_score);
    draw_words((v2){26.f, arena_half_size.y + 6.f}, text_color, "SCORE");
    draw_score((v2){30.f, arena_half_size.y + 3.f}, num_color, (u32)score);

    if (game_over_timer > 0.f) {
        draw_words((v2){-8.f, 0.f}, 0xFEF6EB, "GAME OVER");
    }
}


// ------------------------------------------------------------ agent hook (later)

GameState
get_gamestate_snapshot(void) {
    GameState state = {0};
    state.head_x = head.p.x;
    state.head_y = head.p.y;
    state.dir_x = head.dir.x * head.speed;
    state.dir_y = head.dir.y * head.speed;
    state.food_x = food.p.x;
    state.food_y = food.p.y;
    state.tail_count = tail_count;
    for (int k = 0; k < tail_count; k++) {
        state.tail_positions[2 * k]     = tail[k].p.x;
        state.tail_positions[2 * k + 1] = tail[k].p.y;
    }
    state.lives = lives;
    state.speed = head.speed;
    state.food_eaten = food_eaten;
    state.score = score;
    return state;
}
