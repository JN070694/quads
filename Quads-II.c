/*
 * Quads-II.c
 * Windowed (resizable, never full-screen) character display.
 * Black background, blue lettering.
 * Any letter/number key -> appends that character (up to 3 shown at once).
 * Once 3 characters are showing, further key presses are ignored until
 * Space resets the display back to blank/black.
 * Esc -> quit
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define MAX_CHARS   3
#define FONT_PTSIZE 180

// Common Linux font locations, used as a fallback if no bundled font is
// found next to the executable (see open_font below).
static const char *FONT_CANDIDATES[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    NULL
};

static TTF_Font *open_font(int ptsize) {
    // Prefer a font bundled alongside the executable (AppImage layout:
    // usr/bin/Quads-II + usr/share/quads-ii/DejaVuSans-Bold.ttf) so the
    // app doesn't depend on the host system having a particular font.
    char *base = SDL_GetBasePath(); // e.g. ".../usr/bin/"
    if (base) {
        char path[1024];
        snprintf(path, sizeof(path), "%s../share/quads-ii/DejaVuSans-Bold.ttf", base);
        TTF_Font *f = TTF_OpenFont(path, ptsize);
        SDL_free(base);
        if (f) return f;
    }

    for (int i = 0; FONT_CANDIDATES[i]; i++) {
        TTF_Font *f = TTF_OpenFont(FONT_CANDIDATES[i], ptsize);
        if (f) return f;
    }
    return NULL;
}

// Re-render the current buffer as blue text, centered in the window,
// and blit it against a black background.
static void render_frame(SDL_Renderer *ren, TTF_Font *font, const char *buf) {
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    if (buf[0] != '\0') {
        SDL_Color blue = {0, 0, 255, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(font, buf, blue);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
            if (tex) {
                int win_w, win_h;
                SDL_GetRendererOutputSize(ren, &win_w, &win_h);
                SDL_Rect dst;
                dst.w = surf->w;
                dst.h = surf->h;
                dst.x = (win_w - dst.w) / 2;
                dst.y = (win_h - dst.h) / 2;
                SDL_RenderCopy(ren, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
            }
            SDL_FreeSurface(surf);
        }
    }

    SDL_RenderPresent(ren);
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Windowed, resizable, and maximizable -- but SDL_WINDOW_RESIZABLE
    // alone never goes full-screen, so the desktop, taskbar, and other
    // windows stay reachable and the app is trivial to re-open later.
    SDL_Window *win = SDL_CreateWindow(
        "Quads-II",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIN_WIDTH, WIN_HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = open_font(FONT_PTSIZE);
    if (!font) {
        fprintf(stderr, "Could not find a usable font. Set one of the paths in "
                        "FONT_CANDIDATES to a .ttf file on this system.\n");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    char buf[MAX_CHARS + 1] = {0};
    int len = 0;
    bool dirty = true;
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT &&
                       (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                        e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        e.window.event == SDL_WINDOWEVENT_EXPOSED)) {
                dirty = true;
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode key = e.key.keysym.sym;

                if (key == SDLK_ESCAPE) {
                    quit = true;
                } else if (key == SDLK_SPACE) {
                    len = 0;
                    buf[0] = '\0';
                    dirty = true;
                } else if (len < MAX_CHARS &&
                           ((key >= SDLK_a && key <= SDLK_z) ||
                            (key >= SDLK_0 && key <= SDLK_9))) {
                    buf[len++] = (char)SDL_toupper((int)key);
                    buf[len] = '\0';
                    dirty = true;
                }
                // If len == MAX_CHARS already, non-space keys are ignored
                // until Space resets the buffer.
            }
        }

        if (dirty) {
            render_frame(ren, font, buf);
            dirty = false;
        } else {
            SDL_Delay(10);
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
