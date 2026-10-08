/*
TODO :
-file saving system (high score)
-basic background music
-later try for new levels
-particle system for collision explosion
-add few powerups
*/

global b32 game_initialized = false;
global sim game_state ;

internal void
push_history(sim*s,v2 p) {
    s->path_history[s->history_count % MAX_HISTORY] = p;
    s->history_count++;
}

internal void
update_tail(sim* s) {
    for (int i = 0; i < s->tail_count; i++) {
        int idx = s->history_count - (i + 1);
        if (idx >= 0) s->tail[i] = s->path_history[idx % MAX_HISTORY];
    }
}

internal int
get_region_index(sim* s, v2 p) {
    f32 margin_x = s->arena_half_size.x - s->food_half_size_range.y;
    f32 margin_y = s->arena_half_size.y - s->food_half_size_range.y;

    int col = (int)floorf((p.x + margin_x) / s->region_w);
    int row = (int)floorf((p.y + margin_y) / s->region_h);

    col = clamp_int(0, col, REGION_COLS - 1);
    row = clamp_int(0, row, REGION_ROWS - 1);
    return row * REGION_COLS + col;
}

internal void
update_regions(sim* s) {
    for (int i = 0; i < REGION_COUNT; i++) s->regions[i].occupied = false;

    for (int i = 0; i < s->tail_count; i++) {
        s->regions[get_region_index(s, s->tail[i])].occupied = true;
    }
    s->regions[get_region_index(s, s->Snake_head_p)].occupied = true;
}

internal void
food_spawn(sim* s) {
    update_regions(s);

    // Choose uniformly among FREE regions. The old do/while could never pick
    // region 199 and looped forever once every region was occupied.
    int free_ids[REGION_COUNT];
    int free_count = 0;
    for (int i = 0; i < REGION_COUNT; i++) {
        if (!s->regions[i].occupied) free_ids[free_count++] = i;
    }
    int id = free_count ? free_ids[random_int(free_count)] : random_int(REGION_COUNT);

    s->food_p.x = random_num_generator_in_range(s->regions[id].min.x, s->regions[id].max.x);
    s->food_p.y = random_num_generator_in_range(s->regions[id].min.y, s->regions[id].max.y);
}



// Starts a fresh round. high_score is intentionally NOT touched.
internal void
reset_round(sim* s) {
    s->Snake_head_p = (v2){-20.f, 0.f};
    s->Snake_head_dir = (v2){1.f, 0.f};
    s->Snake_head_last_rec = s->Snake_head_p;
    s->Snake_head_speed = BASE_SPEED;
    s->Snake_head_color = 0x50E66A;

    s->tail_count = START_TAIL;
    s->lives = START_LIVES;
    s->score = 0;
    s->food_eaten = 0;
    s->game_over_timer = 0.f;
    s->dir_queue_count = 0;

    // Lay a straight trail behind the head so the whole tail exists (and is
    // visible, and counts for food placement) from the very first frame.
    s->history_count = 0;
    for (int k = HISTORY_PREFILL; k >= 1; k--) {
        push_history(s, (v2){s->Snake_head_p.x - k * SEGMENT_DIST, s->Snake_head_p.y});
    }
    update_tail(s);

    s->food_half_size = (v2){s->food_half_size_range.y, s->food_half_size_range.y};
    s->food_pulse_dir = -1.f;
    food_spawn(s);
}

internal void
init_game(sim* s) {
    init_seed();
    memset(s, 0, sizeof(sim));
    s->Snake_head_half_size = (v2){1.5f, 1.5f};

    s->food_half_size_range = (v2){0.75f, 2.5f};
    s->food_color = colors[0];

    s->arena_half_size = (v2){85.f, 40.f};
    s->region_w = ((s->arena_half_size.x - s->food_half_size_range.y) * 2) / REGION_COLS;
    s->region_h = ((s->arena_half_size.y - s->food_half_size_range.y) * 2) / REGION_ROWS;

    int idx = 0;
    for (int r = 0; r < REGION_ROWS; r++) {
        for (int c = 0; c < REGION_COLS; c++) {
            s->regions[idx].min = (v2){-(s->arena_half_size.x - s->food_half_size_range.y) + c * s->region_w,
                                       -(s->arena_half_size.y - s->food_half_size_range.y) + r * s->region_h};
            s->regions[idx].max = (v2){s->regions[idx].min.x + s->region_w, s->regions[idx].min.y + s->region_h};
            s->regions[idx].occupied = false;
            idx++;
        }
    }

    game_initialized = true;
    reset_round(s);
}

internal void
begin_game_over(sim* s) {
    s->game_over_timer = GAME_OVER_TIME;
}


// ------------------------------------------------------------ input

// Queue a turn, validated against the heading it will follow (the last queued
// turn, or the current heading). Only 90-degree turns are accepted, so the
// snake can never reverse into itself, no matter how fast keys are mashed.
internal void
queue_direction(sim* s, v2 d) {
    if (s->dir_queue_count >= DIR_QUEUE_MAX) return;
    v2 ref = s->dir_queue_count ? s->dir_queue[s->dir_queue_count - 1] : s->Snake_head_dir;
    f32 dot = ref.x * d.x + ref.y * d.y;
    if (fabsf(dot) < 0.5f) s->dir_queue[s->dir_queue_count++] = d;
}


// ------------------------------------------------------------ simulation
internal void
simulate(sim* s,f32 dt) {
  
    s->Snake_head_p.x += s->Snake_head_dir.x * s->Snake_head_speed * dt;
    s->Snake_head_p.y += s->Snake_head_dir.y * s->Snake_head_speed * dt;

    // --- arena walls (the head stays fully inside the play area) ---
    f32 lim_x = s->arena_half_size.x - s->Snake_head_half_size.x;
    f32 lim_y = s->arena_half_size.y - s->Snake_head_half_size.y;
#if WALLS_KILL
    if (s->Snake_head_p.x > lim_x || s->Snake_head_p.x < -lim_x || s->Snake_head_p.y > lim_y || s->Snake_head_p.y < -lim_y) {
        begin_game_over();
        return;
    }
#else
    b32 wrapped = false;
    if (s->Snake_head_p.x > lim_x)       { s->Snake_head_p.x -= 2.f * lim_x; wrapped = true; }
    else if (s->Snake_head_p.x < -lim_x) { s->Snake_head_p.x += 2.f * lim_x; wrapped = true; }
    if (s->Snake_head_p.y > lim_y)       { s->Snake_head_p.y -= 2.f * lim_y; wrapped = true; }
    else if (s->Snake_head_p.y < -lim_y) { s->Snake_head_p.y += 2.f * lim_y; wrapped = true; }
    if (wrapped) s->Snake_head_last_rec = s->Snake_head_p;   // don't interpolate across the arena
#endif

    // --- record the path at exact SEGMENT_DIST intervals, so the tail spacing
    //     does not depend on frame rate ---
    for (int guard = 0; guard < 16; guard++) {
        f32 dx = s->Snake_head_p.x - s->Snake_head_last_rec.x;
        f32 dy = s->Snake_head_p.y - s->Snake_head_last_rec.y;
        f32 d2 = dx * dx + dy * dy;
        if (d2 < SEGMENT_DIST * SEGMENT_DIST) break;
        f32 d = sqrtf(d2);
        s->Snake_head_last_rec.x += dx / d * SEGMENT_DIST;
        s->Snake_head_last_rec.y += dy / d * SEGMENT_DIST;
        push_history(s,s->Snake_head_last_rec);
    }
    update_tail(s);

    // --- self collision: cut the tail at the hit segment, lose a life ---
    for (int i = SAFE_TAIL_SEGMENTS; i < s->tail_count; i++) {
        if (is_colliding(s->Snake_head_p, s->tail[i], s->Snake_head_half_size, s->Snake_head_half_size)) {
            s->tail_count = i;
            s->Snake_head_color = random_color();
            s->lives--;
            if (s->lives <= 0) {
                s->lives = 0;
                begin_game_over(s);
                return;
            }
            break;
        }
    }

    // --- food pulse (time based, flips colour at each extreme) ---
    f32 size = s->food_half_size.x + s->food_pulse_dir * FOOD_PULSE_SPEED * dt;
    if (size >= s->food_half_size_range.y) {
        size = s->food_half_size_range.y;
        s->food_pulse_dir = -1.f;
        s->food_color = colors[(s->food_color_idx + 1) % MAX_COLORS];
        s->food_color_idx = (s->food_color_idx + 1) % MAX_COLORS;
    } else if (size <= s->food_half_size_range.x) {
        size = s->food_half_size_range.x;
        s->food_pulse_dir = 1.f;
        s->food_color = colors[(s->food_color_idx + 1) % MAX_COLORS];
        s->food_color_idx = (s->food_color_idx + 1) % MAX_COLORS;
    }
    s->food_half_size = (v2){size, size};

    // --- eating ---
    if (is_colliding(s->Snake_head_p, s->food_p, s->Snake_head_half_size, s->food_half_size)) {
        s->food_eaten++;
        s->score += s->tail_count;
        if (s->score > s->high_score) s->high_score = s->score;
        s->Snake_head_speed = clamp(BASE_SPEED, s->Snake_head_speed + s->tail_count / 50.f, MAX_SPEED);

        if (s->tail_count < MAX_TAIL) {
            s->tail[s->tail_count] = s->tail_count > 0 ? s->tail[s->tail_count - 1]: s->Snake_head_p;
            s->tail_count++;
        }
        food_spawn(s);
    }
}

internal void
update_game(Input *input, f32 dt){
    sim* s = &game_state;
    if (!game_initialized) init_game(s);
    if (s->game_over_timer > 0.f) {
        s->game_over_timer -= dt;
        if (s->game_over_timer <= 0.f) reset_round(s);
        return;
    }
    if (pressed(BUTTON_UP))    queue_direction(s, (v2){0.f, 1.f});
    if (pressed(BUTTON_DOWN))  queue_direction(s, (v2){0.f, -1.f});
    if (pressed(BUTTON_LEFT))  queue_direction(s, (v2){-1.f, 0.f});
    if (pressed(BUTTON_RIGHT)) queue_direction(s, (v2){1.f, 0.f});
    if (s->dir_queue_count > 0) {
        s->Snake_head_dir = s->dir_queue[0];
        for (int i = 1; i < s->dir_queue_count; i++) s->dir_queue[i - 1] = s->dir_queue[i];
        s->dir_queue_count--;
    }
    simulate(s,dt);
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
render_game() {
    sim * s = &game_state;
    clear_screen(0xE69F50);
    draw((v2){0.f, 0.f}, s->arena_half_size, 0x262D27, SHAPE_RECT);

    // lives
    v2 heart_p = (v2){-s->arena_half_size.x + 3.f, s->arena_half_size.y + 3.f};
    for (int i = 0; i < s->lives; i++) {
        draw_heart(heart_p, 0xEA2323);
        heart_p.x += 5.f;
    }

    // tail (oldest first so newer segments overlap older ones), then head
    for (int i = s->tail_count - 1; i >= 0; i--) {
        draw(s->tail[i], s->Snake_head_half_size, s->Snake_head_color, SHAPE_CIRCLE);
    }
    draw(s->Snake_head_p, s->Snake_head_half_size, s->Snake_head_color, SHAPE_CIRCLE);

    draw(s->food_p, s->food_half_size, s->food_color, SHAPE_CIRCLE);

    // HUD
    u32 text_color = 0x4A1D54;
    u32 num_color = 0xFEF6EB;
    draw_words((v2){-s->arena_half_size.x + 4.f, s->arena_half_size.y + 6.f}, text_color, "LIVES");
    draw_words((v2){-8.f, s->arena_half_size.y + 6.f}, text_color, "HIGH SCORE");
    draw_score((v2){0.f, s->arena_half_size.y + 3.f}, num_color, (u32)s->high_score);
    draw_words((v2){26.f, s->arena_half_size.y + 6.f}, text_color, "SCORE");
    draw_score((v2){30.f, s->arena_half_size.y + 3.f}, num_color, (u32)s->score);

    if (s->game_over_timer > 0.f) {
        draw_words((v2){-8.f, 0.f}, 0xFEF6EB, "GAME OVER");
    }
}


