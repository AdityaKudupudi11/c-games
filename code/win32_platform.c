#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "utils.c"
#include "platform_common.c"
#include "math.c"

typedef struct {
    // Platform non-specific part
    int width, height;
    u32 *pixels;
    BITMAPINFO bitmap_info;
} Render_Buffer;

static Render_Buffer render_buffer;
#include "collision.c"
#include "software_rendering.c"
#include "console.c"
#include "sim.h"
#include "game.c"

#define SIM_DT (1.0f / 60.0f)     // fixed simulation step

internal LRESULT CALLBACK
window_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {

    LRESULT result = 0;

    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY: {
            running = false;
        } break;

        case WM_SIZE: {
            // Client area, not the window rect (which includes title bar and borders).
            RECT rect;
            GetClientRect(window, &rect);
            int new_w = rect.right - rect.left;
            int new_h = rect.bottom - rect.top;

            // Minimised window: keep the old buffer instead of allocating 0 bytes.
            if (new_w <= 0 || new_h <= 0) break;
            if (render_buffer.pixels && new_w == render_buffer.width && new_h == render_buffer.height) break;

            u32 *new_pixels = VirtualAlloc(0, sizeof(u32) * new_w * new_h,
                                           MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!new_pixels) break;
            if (render_buffer.pixels) VirtualFree(render_buffer.pixels, 0, MEM_RELEASE);

            render_buffer.pixels = new_pixels;
            render_buffer.width = new_w;
            render_buffer.height = new_h;

            render_buffer.bitmap_info.bmiHeader.biSize = sizeof(render_buffer.bitmap_info.bmiHeader);
            render_buffer.bitmap_info.bmiHeader.biWidth = new_w;
            render_buffer.bitmap_info.bmiHeader.biHeight = new_h;     // positive = bottom-up, so +y is up
            render_buffer.bitmap_info.bmiHeader.biPlanes = 1;
            render_buffer.bitmap_info.bmiHeader.biBitCount = 32;
            render_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB;
        } break;

        default: {
            result = DefWindowProcA(window, message, w_param, l_param);
        }
    }

    return result;
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nShowCmd;

    timeBeginPeriod(1);    // 1ms Sleep() granularity

    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = window_callback;
    window_class.lpszClassName = "Game_Window_Class";
    window_class.hCursor = LoadCursor(0, IDC_ARROW);

    RegisterClassA(&window_class);

    HWND window = CreateWindowExA(0, window_class.lpszClassName, "Snake",
                                  WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, 0, 0);
    if (!window) return 1;
    HDC hdc = GetDC(window);

    Input input = {0};

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER last_counter;
    QueryPerformanceCounter(&last_counter);

    f64 accumulator = 0.0;

    while (running) {
        // ---- Input / window messages ----
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {   // 0 = all windows + thread messages
            switch (message.message) {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP: {
                    u32 vk_code = (u32)message.wParam;
                    u32 lp = (u32)message.lParam;
                    b32 key_down = ((lp >> 31) & 1) == 0;     // transition bit: 0 = pressed

#define process_button(vk, b) \
                    if (vk_code == (vk)) { \
                        if (key_down && !input.buttons[b].is_down) input.buttons[b].was_pressed = true; \
                        input.buttons[b].changed = key_down != input.buttons[b].is_down; \
                        input.buttons[b].is_down = key_down; \
                    }

                    process_button(VK_UP,    BUTTON_UP);
                    process_button(VK_DOWN,  BUTTON_DOWN);
                    process_button(VK_LEFT,  BUTTON_LEFT);
                    process_button(VK_RIGHT, BUTTON_RIGHT);
                    process_button('W', BUTTON_UP);
                    process_button('S', BUTTON_DOWN);
                    process_button('A', BUTTON_LEFT);
                    process_button('D', BUTTON_RIGHT);

                    if (vk_code == VK_ESCAPE && key_down) running = false;
                } break;
                default: break;
            }
            // Always forward the message, including SYSKEY ones: swallowing them
            // is what broke Alt+F4.
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // ---- Timing: fixed-step simulation, clamped so a window drag can't
        //      produce a huge dt and teleport the snake ----
        LARGE_INTEGER current_counter;
        QueryPerformanceCounter(&current_counter);
        f64 frame_time = (f64)(current_counter.QuadPart - last_counter.QuadPart) / (f64)frequency.QuadPart;
        last_counter = current_counter;
        if (frame_time > 0.25) frame_time = 0.25;
        accumulator += frame_time;

        if (accumulator < SIM_DT) {
            Sleep(1);      // don't burn a whole core spinning
            continue;
        }

        // ---- Simulation ----
        while (accumulator >= SIM_DT) {
            update_game(&input, SIM_DT);
            accumulator -= SIM_DT;

            // Press latches live for exactly one tick.
            for (int i = 0; i < BUTTON_COUNT; i++) {
                input.buttons[i].changed = false;
                input.buttons[i].was_pressed = false;
            }
        }

        // ---- Render ----
        render_game();
        if (render_buffer.pixels) {
            StretchDIBits(hdc, 0, 0, render_buffer.width, render_buffer.height,
                          0, 0, render_buffer.width, render_buffer.height,
                          render_buffer.pixels, &render_buffer.bitmap_info,
                          DIB_RGB_COLORS, SRCCOPY);
        }
    }

    timeEndPeriod(1);
    return 0;
}
