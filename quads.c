/*
 * quads.c
 * Full-screen 4-quadrant clicker
 * 1,2,3,4 (numpad or number row) -> set quadrant color
 * Space -> reset all to black
 * Esc -> exit full-screen & quit
 * Mouse cursor is hidden while running
 */
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH  0  // 0 = use current display width
#define SCREEN_HEIGHT 0  // 0 = use current display height

typedef enum {
    Q1 = 0, // top-left     -> Red
    Q2,     // top-right    -> White
    Q3,     // bottom-left  -> Blue
    Q4      // bottom-right -> Green
} Quad;

// Helper macro: set color to (r,g,b) when active, else black
#define SET_COLOR(q, r, g, b) \
SDL_SetRenderDrawColor(ren, active[q] ? (r) : 0, active[q] ? (g) : 0, active[q] ? (b) : 0, 255)

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Prevent window minimize flicker
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        fprintf(stderr, "SDL_GetCurrentDisplayMode Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    int w = (SCREEN_WIDTH > 0) ? SCREEN_WIDTH : dm.w;
    int h = (SCREEN_HEIGHT > 0) ? SCREEN_HEIGHT : dm.h;

    SDL_Window *win = SDL_CreateWindow(
        "4-Quadrant Clicker",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_ShowCursor(SDL_DISABLE); // Hide cursor immediately

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_Quit();
        return 1;
    }

    // All quadrants start inactive (black)
    bool active[4] = {false};
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_SPACE:
                        for (int i = 0; i < 4; i++) active[i] = false;
                        break;
                    case SDLK_KP_1: case SDLK_1: active[Q1] = true; break;
                    case SDLK_KP_2: case SDLK_2: active[Q2] = true; break;
                    case SDLK_KP_3: case SDLK_3: active[Q3] = true; break;
                    case SDLK_KP_4: case SDLK_4: active[Q4] = true; break;
                }
            }
        }

        // ----- Render -----
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        int halfW = w / 2;
        int halfH = h / 2;
        SDL_Rect rect;

        // Q1: Top-left -> Red
        SET_COLOR(Q1, 255, 0, 0);
        rect = (SDL_Rect){0, 0, halfW, halfH};
        SDL_RenderFillRect(ren, &rect);

        // Q2: Top-right -> White
        SET_COLOR(Q2, 255, 255, 255);
        rect = (SDL_Rect){halfW, 0, w - halfW, halfH};
        SDL_RenderFillRect(ren, &rect);

        // Q3: Bottom-left -> Blue
        SET_COLOR(Q3, 0, 0, 255);
        rect = (SDL_Rect){0, halfH, halfW, h - halfH};
        SDL_RenderFillRect(ren, &rect);

        // Q4: Bottom-right -> Green
        SET_COLOR(Q4, 0, 255, 0);
        rect = (SDL_Rect){halfW, halfH, w - halfW, h - halfH};
        SDL_RenderFillRect(ren, &rect);

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_ShowCursor(SDL_ENABLE); // Restore cursor
    SDL_Quit();
    return 0;
}
