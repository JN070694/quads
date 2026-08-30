/*
 * Quads-III.c
 * Windowed (resizable, never full-screen) character display.
 * Black background, large orange lettering used as visual signals.
 * Any letter/number key -> appends that character (up to 3 shown at once).
 * Once 3 characters are showing, further key presses are ignored until
 * Space resets the display back to blank/black.
 * Text auto-scales to stay as large as possible for the current window
 * size, so it stays legible whether the window is small or maximized.
 * Closing the window (X button) is the only way to quit; Esc no longer
 * closes the app so it doesn't get triggered while it's being used.
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WIN_WIDTH     800
#define WIN_HEIGHT    600
#define MAX_CHARS     3
#define MARGIN_FRAC   0.90  // fraction of window each dimension the text may fill
#define MIN_PTSIZE    12
#define MAX_PTSIZE    2000

static char FONT_PATH[1024];
static bool FONT_FOUND = false;

// Common Linux font locations, used as a fallback if no bundled font is
// found next to the executable.
static const char *FONT_CANDIDATES[] = {
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
    NULL
};

// Locate a usable font file once at startup and remember its path.
static bool find_font_path(char *out, size_t outsz) {
    // Prefer a font bundled alongside the executable (AppImage layout:
    // usr/bin/Quads-III + usr/share/quads-iii/LiberationSans-Bold.ttf) so
    // the app doesn't depend on the host system having a particular font.
    char *base = SDL_GetBasePath(); // e.g. ".../usr/bin/"
    if (base) {
        char path[1024];
        snprintf(path, sizeof(path), "%s../share/quads-iii/LiberationSans-Bold.ttf", base);
        TTF_Font *f = TTF_OpenFont(path, 12);
        if (f) {
            TTF_CloseFont(f);
            SDL_free(base);
            snprintf(out, outsz, "%s", path);
            return true;
        }
        SDL_free(base);
    }

    for (int i = 0; FONT_CANDIDATES[i]; i++) {
        TTF_Font *f = TTF_OpenFont(FONT_CANDIDATES[i], 12);
        if (f) {
            TTF_CloseFont(f);
            snprintf(out, outsz, "%s", FONT_CANDIDATES[i]);
            return true;
        }
    }
    return false;
}

// Open the font at the largest point size that fits the given text within
// (max_w, max_h), using a couple of measure-and-adjust passes.
static TTF_Font *open_fitted_font(const char *text, int max_w, int max_h) {
    int ptsize = max_h; // initial guess: text height ~= point size
    if (ptsize > MAX_PTSIZE) ptsize = MAX_PTSIZE;
    if (ptsize < MIN_PTSIZE) ptsize = MIN_PTSIZE;

    TTF_Font *font = NULL;
    for (int attempt = 0; attempt < 4; attempt++) {
        if (font) TTF_CloseFont(font);
        font = TTF_OpenFont(FONT_PATH, ptsize);
        if (!font) return NULL;

        int tw = 0, th = 0;
        TTF_SizeText(font, text, &tw, &th);
        if (tw <= 0) tw = 1;
        if (th <= 0) th = 1;

        double scale_w = (double)max_w / tw;
        double scale_h = (double)max_h / th;
        double scale = scale_w < scale_h ? scale_w : scale_h;

        if (scale > 0.98 && scale < 1.02) break; // close enough

        int next = (int)(ptsize * scale);
        if (next < MIN_PTSIZE) next = MIN_PTSIZE;
        if (next > MAX_PTSIZE) next = MAX_PTSIZE;
        if (next == ptsize) break; // no progress, stop
        ptsize = next;
    }
    return font;
}

// Re-render the current buffer as large orange text, centered in the
// window and scaled to fill it, against a black background.
static void render_frame(SDL_Renderer *ren, const char *buf) {
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    if (buf[0] != '\0') {
        int win_w, win_h;
        SDL_GetRendererOutputSize(ren, &win_w, &win_h);

        int max_w = (int)(win_w * MARGIN_FRAC);
        int max_h = (int)(win_h * MARGIN_FRAC);

        TTF_Font *font = open_fitted_font(buf, max_w, max_h);
        if (font) {
            SDL_Color orange = {255, 140, 0, 255};
            SDL_Surface *surf = TTF_RenderText_Blended(font, buf, orange);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
                if (tex) {
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
            TTF_CloseFont(font);
        }
    }

    SDL_RenderPresent(ren);
}

// Map an SDL keycode to the display character it represents, or 0 if the
// key isn't one we display. Handles both the top-row number keys and the
// numeric keypad, which use entirely different SDL keycodes.
static char key_to_char(SDL_Keycode key) {
    if (key >= SDLK_a && key <= SDLK_z) return (char)('A' + (key - SDLK_a));
    if (key >= SDLK_0 && key <= SDLK_9) return (char)('0' + (key - SDLK_0));
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9) return (char)('1' + (key - SDLK_KP_1));
    if (key == SDLK_KP_0) return '0';
    return 0;
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

    if (!find_font_path(FONT_PATH, sizeof(FONT_PATH))) {
        fprintf(stderr, "Could not find a usable font. Set one of the paths in "
                        "FONT_CANDIDATES to a .ttf file on this system.\n");
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    FONT_FOUND = true;

    // Windowed, resizable, and maximizable -- but SDL_WINDOW_RESIZABLE
    // alone never goes full-screen, so the desktop, taskbar, and other
    // windows stay reachable and the app is trivial to re-open later.
    SDL_Window *win = SDL_CreateWindow(
        "Quads-III",
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

                if (key == SDLK_SPACE) {
                    len = 0;
                    buf[0] = '\0';
                    dirty = true;
                } else {
                    char c = key_to_char(key);
                    if (c && len < MAX_CHARS) {
                        buf[len++] = c;
                        buf[len] = '\0';
                        dirty = true;
                    }
                }
                // If len == MAX_CHARS already, non-space keys are ignored
                // until Space resets the buffer.
            }
        }

        if (dirty) {
            render_frame(ren, buf);
            dirty = false;
        } else {
            SDL_Delay(10);
        }
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
