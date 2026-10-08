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