typedef enum {
    SHAPE_RECT,
    SHAPE_CIRCLE,
} Shape;

internal void
clear_screen(u32 color) {
    if (!render_buffer.pixels) return;
    u32 *pixel = render_buffer.pixels;
    int count = render_buffer.width * render_buffer.height;
    for (int i = 0; i < count; i++) *pixel++ = color;
}

// Fills pixels [x0,x1) x [y0,y1). End coordinates are exclusive.
internal void
draw_rect(int x0, int y0, int x1, int y1, u32 color) {
    if (!render_buffer.pixels) return;

    x0 = clamp_int(0, x0, render_buffer.width);
    x1 = clamp_int(0, x1, render_buffer.width);
    y0 = clamp_int(0, y0, render_buffer.height);
    y1 = clamp_int(0, y1, render_buffer.height);

    for (int y = y0; y < y1; y++) {
        u32 *pixel = render_buffer.pixels + x0 + render_buffer.width * y;
        for (int x = x0; x < x1; x++) {
            *pixel++ = color;
        }
    }
}

// Same exclusive-end convention as draw_rect. Pixel centres are tested against
// the radius, so a circle is never bigger than the rect that bounds it.
internal void
draw_circle(int x0, int y0, int x1, int y1, v2 center, f32 radius, u32 color) {
    if (!render_buffer.pixels) return;

    x0 = clamp_int(0, x0, render_buffer.width);
    x1 = clamp_int(0, x1, render_buffer.width);
    y0 = clamp_int(0, y0, render_buffer.height);
    y1 = clamp_int(0, y1, render_buffer.height);

    f32 r2 = radius * radius;
    for (int y = y0; y < y1; y++) {
        u32 *pixel = render_buffer.pixels + x0 + render_buffer.width * y;
        f32 dy = ((f32)y + 0.5f) - center.y;
        for (int x = x0; x < x1; x++) {
            f32 dx = ((f32)x + 0.5f) - center.x;
            if (dx * dx + dy * dy <= r2) *pixel = color;
            pixel++;
        }
    }
}

// p and half_size are in "world units" (the arena is 170 x 80 units); this
// converts them to pixels, keeping a 1.77 aspect ratio, and draws the shape.
internal void
draw(v2 p, v2 half_size, u32 color, Shape shape) {
    if (!render_buffer.pixels) return;

    f32 aspect_multiplier = (f32)render_buffer.height;
    if ((f32)render_buffer.width / (f32)render_buffer.height < 1.77f)
        aspect_multiplier = (f32)render_buffer.width / 1.77f;

    f32 scale = 0.01f;
    half_size.x *= aspect_multiplier * scale;
    half_size.y *= aspect_multiplier * scale;

    p.x *= aspect_multiplier * scale;
    p.y *= aspect_multiplier * scale;

    p.x += (f32)render_buffer.width * .5f;
    p.y += (f32)render_buffer.height * .5f;

    // floor/ceil instead of (int) truncation: truncation rounds toward zero,
    // which shifts shapes by 1px on one side of the screen centre.
    int x0 = (int)floorf(p.x - half_size.x);
    int y0 = (int)floorf(p.y - half_size.y);
    int x1 = (int)ceilf(p.x + half_size.x);
    int y1 = (int)ceilf(p.y + half_size.y);

    if (shape == SHAPE_RECT) {
        draw_rect(x0, y0, x1, y1, color);
    } else {
        draw_circle(x0, y0, x1, y1, p, half_size.x, color);
    }
}
