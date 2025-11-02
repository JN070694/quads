/*
 * quads.c
 * Full-screen 4-quadrant clicker
 * 1,2,3,4 (numpad) -> green
 * Esc -> exit full-screen & quit
 */

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH  0   // 0 = use current display width
#define SCREEN_HEIGHT 0   // 0 = use current display height

typedef enum {
    Q1 = 0,   // top-left
    Q2,       // top-right
    Q3,       // bottom-left
    Q4        // bottom-right
} Quad;

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Get current display mode to compute real size
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    int w = (SCREEN_WIDTH  > 0) ? SCREEN_WIDTH  : dm.w;
    int h = (SCREEN_HEIGHT > 0) ? SCREEN_HEIGHT : dm.h;

    // Fullscreen window
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

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // State: which quadrants are green?
    bool green[4] = {false, false, false, false};

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
                        green[Q1] = false;
                        green[Q2] = false;
                        green[Q3] = false;
                        green[Q4] = false;
                        break;
                        // Numeric keypad keys
                    case SDLK_KP_1: green[Q1] = true; break; // top-left
                    case SDLK_KP_2: green[Q2] = true; break; // top-right
                    case SDLK_KP_3: green[Q3] = true; break; // bottom-left
                    case SDLK_KP_4: green[Q4] = true; break; // bottom-right
                    // Fallback for normal number row
                    case SDLK_1: green[Q1] = true; break;
                    case SDLK_2: green[Q2] = true; break;
                    case SDLK_3: green[Q3] = true; break;
                    case SDLK_4: green[Q4] = true; break;
                }
            }
        }

        // ----- Render -----
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);   // black background
        SDL_RenderClear(ren);

        int halfW = w / 2;
        int halfH = h / 2;

        // Helper to draw a quad
        SDL_Rect rect;

        if (green[Q1]) SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
        else           SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        rect.x = 0; rect.y = 0; rect.w = halfW; rect.h = halfH;
        SDL_RenderFillRect(ren, &rect);

        if (green[Q2]) SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
        else           SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        rect.x = halfW; rect.y = 0; rect.w = w - halfW; rect.h = halfH;
        SDL_RenderFillRect(ren, &rect);

        if (green[Q3]) SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
        else           SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        rect.x = 0; rect.y = halfH; rect.w = halfW; rect.h = h - halfH;
        SDL_RenderFillRect(ren, &rect);

        if (green[Q4]) SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
        else           SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        rect.x = halfW; rect.y = halfH; rect.w = w - halfW; rect.h = h - halfH;
        SDL_RenderFillRect(ren, &rect);

        SDL_RenderPresent(ren);
        // ------------------
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
