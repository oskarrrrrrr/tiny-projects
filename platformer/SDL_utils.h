#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "types.h"

typedef struct {
    SDL_Renderer *renderer;
    f32 xs, ys;
} SDL_ScaledRenderer;

typedef struct {
    SDL_ScaledRenderer scaled_renderer;
    SDL_Window *window;
    int w, h;
} Window;

void SDL_fail(void);
void SDL_init(
    SDL_Window **window,
    SDL_Renderer **renderer,
    int initial_width,
    int initial_height
);
void SDL_destroy(SDL_Window **window, SDL_Renderer **renderer);
void SDL_print_rect(SDL_Rect rect);
void SDL_get_window_scale(SDL_Window *window, SDL_Renderer *renderer, f32 *xs, f32 *ys);
SDL_Rect SDL_ScaleRect(SDL_ScaledRenderer scaled_renderer, SDL_Rect rect);

// ScaledRenderer
int SDL_ScaledRenderDrawLine(SDL_ScaledRenderer scaled_renderer, int x1, int y1, int x2, int y2);
int SDL_ScaledRenderFillRect(SDL_ScaledRenderer scaled_renderer, const SDL_Rect *rect);
int SDL_ScaledRenderDrawRect(SDL_ScaledRenderer scaled_renderer, const SDL_Rect *rect);
int SDL_ScaledRenderCopy(
    SDL_ScaledRenderer scaled_renderer,
    SDL_Texture *texture,
    const SDL_Rect *srcrect,
    const SDL_Rect *dstrect
);
int SDL_QueryScaledTexture(
    SDL_ScaledRenderer scaled_renderer,
    SDL_Texture *texture,
    u32 *format,
    int *access,
    int *w,
    int *h
);

// TTF
TTF_Font *TTF_open_font(
    SDL_ScaledRenderer scaled_renderer,
    char *font_file_name,
    int font_size
);

#endif // SDL_UTILS_H
