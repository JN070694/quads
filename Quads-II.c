/*
 * Quads-II.c
 * Full-screen line clicker
 * 1,a -> two parallel RED lines, bottom-left to top-right (positive slope)
 * 2,b -> two parallel RED lines, top-left to bottom-right (negative slope)
 * 3,c -> same as 1/a but BLUE
 * 4,d -> same as 2/b but BLUE
 * 5,e -> two parallel flat GREEN lines, centered on screen
 * Any overlap between differently-colored lines turns YELLOW
 * Space -> reset all to BLACK
 * Esc -> exit full-screen & quit
 * Mouse cursor is hidden while running
 */
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH  0
#define SCREEN_HEIGHT 0
#define NUM_GROUPS 5
#define THICK 8
#define GAP   60

static Uint32 BLACK, RED, BLUE, GREEN, YELLOW;

static inline void plot(Uint32 *px, int w, int h, int x, int y, Uint32 color) {
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    Uint32 *p = &px[y * w + x];
    if (*p == BLACK) *p = color;
    else if (*p != color) *p = YELLOW;
}

static void draw_hline(Uint32 *px, int w, int h, int y, Uint32 color) {
    for (int t = 0; t < THICK; t++)
        for (int x = 0; x < w; x++)
            plot(px, w, h, x, y + t, color);
}

// Diagonal line from (0,0) to (w,h), shifted down by yoffset. A constant
// vertical shift keeps it exactly parallel to the base diagonal.
static void draw_diag(Uint32 *px, int w, int h, int yoffset, Uint32 color) {
    for (int x = 0; x < w; x++) {
        int y = (int)((long long)x * h / w) + yoffset;
        for (int t = 0; t < THICK; t++)
            plot(px, w, h, x, y + t, color);
    }
}

// Mirror of draw_diag: bottom-left to top-right (positive slope), shifted
// down by yoffset. Same constant-shift trick keeps parallel lines exact.
static void draw_diag_mirror(Uint32 *px, int w, int h, int yoffset, Uint32 color) {
    for (int x = 0; x < w; x++) {
        int y = h - (int)((long long)x * h / w) + yoffset;
        for (int t = 0; t < THICK; t++)
            plot(px, w, h, x, y + t, color);
    }
}

static void rebuild(Uint32 *px, int w, int h, bool *active) {
    for (int i = 0; i < w * h; i++) px[i] = BLACK;

    int gap = (h < GAP * 3) ? h / 4 : GAP;

    if (active[0]) { draw_diag_mirror(px, w, h, 0, RED);   draw_diag_mirror(px, w, h, gap, RED); }
    if (active[1]) { draw_diag(px, w, h, 0, RED);          draw_diag(px, w, h, gap, RED); }
    if (active[2]) { draw_diag_mirror(px, w, h, 0, BLUE);  draw_diag_mirror(px, w, h, gap, BLUE); }
    if (active[3]) { draw_diag(px, w, h, 0, BLUE);         draw_diag(px, w, h, gap, BLUE); }
    if (active[4]) { draw_hline(px, w, h, h / 2 - gap / 2, GREEN); draw_hline(px, w, h, h / 2 + gap / 2, GREEN); }
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0"); // no minimize flicker

    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        fprintf(stderr, "SDL_GetCurrentDisplayMode Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    int w = (SCREEN_WIDTH > 0) ? SCREEN_WIDTH : dm.w;
    int h = (SCREEN_HEIGHT > 0) ? SCREEN_HEIGHT : dm.h;

    SDL_Window *win = SDL_CreateWindow(
        "Quads-II",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_ShowCursor(SDL_DISABLE);

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_Quit();
        return 1;
    }

    SDL_PixelFormat *fmt = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
    BLACK  = SDL_MapRGBA(fmt, 0,   0,   0,   255);
    RED    = SDL_MapRGBA(fmt, 255, 0,   0,   255);
    BLUE   = SDL_MapRGBA(fmt, 0,   0,   255, 255);
    GREEN  = SDL_MapRGBA(fmt, 0,   255, 0,   255);
    YELLOW = SDL_MapRGBA(fmt, 255, 255, 0,   255);
    SDL_FreeFormat(fmt);

    SDL_Texture *canvas = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!canvas) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_Quit();
        return 1;
    }

    Uint32 *pixels = malloc((size_t)w * h * sizeof(Uint32));
    if (!pixels) {
        fprintf(stderr, "Out of memory\n");
        SDL_DestroyTexture(canvas);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_Quit();
        return 1;
    }

    bool active[NUM_GROUPS] = {false};
    bool dirty = true; // draw the initial (blank) frame once
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_SPACE:
                        for (int i = 0; i < NUM_GROUPS; i++) active[i] = false;
                        dirty = true;
                        break;
                    case SDLK_KP_1: case SDLK_1: case SDLK_a: active[0] = true; dirty = true; break;
                    case SDLK_KP_2: case SDLK_2: case SDLK_b: active[1] = true; dirty = true; break;
                    case SDLK_KP_3: case SDLK_3: case SDLK_c: active[2] = true; dirty = true; break;
                    case SDLK_KP_4: case SDLK_4: case SDLK_d: active[3] = true; dirty = true; break;
                    case SDLK_KP_5: case SDLK_5: case SDLK_e: active[4] = true; dirty = true; break;
                }
            }
        }

        // Only rebuild the pixel buffer when the active set actually
        // changes, then just re-blit the cached texture every frame.
        if (dirty) {
            rebuild(pixels, w, h, active);
            SDL_UpdateTexture(canvas, NULL, pixels, w * (int)sizeof(Uint32));
            dirty = false;
        }

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, canvas, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    free(pixels);
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_ShowCursor(SDL_ENABLE);
    SDL_Quit();
    return 0;
}
