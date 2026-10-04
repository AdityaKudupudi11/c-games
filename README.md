# Snake (Win32 software renderer)

A new Snake game written in plain C for Windows. There is no game engine and no GPU API: everything is drawn into a 32-bit pixel buffer on the CPU and copied to the window with GDI.

## Controls

| Key | Action |
|---|---|
| Arrow keys / WASD | Turn the snake |
| Esc | Quit |

## Gameplay

- The snake moves continuously and turns in 90° steps. It can never reverse into itself.
- Eat the pulsing food to grow by one segment, score points (your current tail length), and speed up slightly.
- Hitting your own tail cuts it off at the segment you hit and costs one life. You start with 3 lives.
- At 0 lives, "GAME OVER" is shown for 1.5 seconds and a new round begins. The high score is kept for the session.
- By default the arena **wraps around**: leaving one side brings you back on the opposite side. Set `WALLS_KILL 1` in `game.c` to make walls end the round instead.

## Build and run

Requirements: Windows and the MSVC compiler (`cl`). Run from a **Developer Command Prompt for Visual Studio** so `cl` is on the PATH.

```bat
run.bat
```

This runs `code\build.bat` and then launches `build\win32_platform.exe`. The build is a single translation unit: `win32_platform.c` includes all the other `.c` files.

## Project layout

```
run.bat                 build + run
code/
  win32_platform.c      entry point, window, input, main loop (fixed 60 Hz step)
  game.c                game state, simulation (update_game), drawing (render_game)
  software_rendering.c  pixel buffer: clear, rect, circle, world->screen transform
  console.c             5x5 block font for text and numbers
  collision.c           AABB overlap test
  math.c                RNG (xorshift32), clamp, v2 helpers
  platform_common.c     Input / Button types
  utils.c               typedefs (u32, f32, v2 ...) and macros
  build.bat             MSVC build script
```

## How it works

**Main loop.** Window messages are read, then the simulation runs in fixed 1/60 s steps with an accumulator, so game speed does not depend on frame rate. The frame is rendered once per loop pass and copied to the window with `StretchDIBits`.

**Coordinates.** The game works in "world units". The arena is 170 x 80 units (half size 85 x 40) centred on (0,0) with +y up. `draw()` converts world units to pixels and keeps a 1.77 aspect ratio.

**Tail.** The head leaves a breadcrumb in a ring buffer (`path_history`) every 1.5 units along its path. Tail segment `i` is placed on the i-th most recent breadcrumb, so the tail follows every turn exactly and its spacing is independent of frame rate. Eating adds a segment, and a self-hit cuts the tail at that segment.

**Turning.** Key presses are queued (up to 3). Only 90° turns relative to the last queued direction are accepted, and one turn is applied per tick.

**Food placement.** The arena is divided into a 20 x 10 grid of regions. Food spawns in a random region that the snake is not occupying.

## Configuration (top of `game.c`)

| Macro | Default | Meaning |
|---|---|---|
| `WALLS_KILL` | 0 | 0 = wrap around, 1 = walls end the round |
| `START_TAIL` | 7 | Starting number of tail segments |
| `START_LIVES` | 3 | Lives per round |
| `BASE_SPEED` / `MAX_SPEED` | 30 / 90 | Head speed range (units per second) |
| `SEGMENT_DIST` | 1.5 | Distance between tail segments |
| `SAFE_TAIL_SEGMENTS` | 5 | First N segments can never hit the head |
| `GAME_OVER_TIME` | 1.5 | Seconds the game-over banner shows |

## Roadmap

- [ ] Save the high score to a file
- [ ] Background music
- [ ] Particle effects on collisions
- [ ] Power-ups and new levels
