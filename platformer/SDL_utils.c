#include <stdio.h>
#include "SDL_utils.h"

void SDL_fail() {
    printf("SDL ERROR: %s\n", SDL_GetError());
    exit(1);
}

void SDL_init(
    SDL_Window **window,
    SDL_Renderer **renderer,
    int initial_width,
    int initial_height
) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { SDL_fail(); }
    *window = SDL_CreateWindow(
        "Platformer",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        initial_width, initial_height,
        SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!*window) { SDL_fail(); }
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) { SDL_fail(); }

    TTF_Init();
}

void SDL_destroy(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_DestroyRenderer(*renderer);
    *renderer = NULL;
    SDL_DestroyWindow(*window);
    *window = NULL;
    TTF_Quit();
}

void SDL_print_rect(SDL_Rect rect) {
    printf("Rect{x: %d, y: %d, w: %d, h: %d}\n", rect.x, rect.y, rect.w, rect.h);
}

void SDL_get_window_scale(
    SDL_Window *window,
    SDL_Renderer *renderer,
    f32 *xs,
    f32 *ys
) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    int rendererWidth, rendererHeight;
    SDL_GetRendererOutputSize(renderer, &rendererWidth, &rendererHeight);
    if (rendererWidth != windowWidth) {
        *xs = ((f32)rendererWidth) / windowWidth;
        *ys = ((f32)rendererHeight) / windowHeight;
    } else {
        *xs = 1;
        *ys = 1;
    }
}

int SDL_ScaledRenderDrawLine(SDL_ScaledRenderer scaled_renderer, int x1, int y1, int x2, int y2) {
    return SDL_RenderDrawLine(
        scaled_renderer.renderer,
        scaled_renderer.xs * x1,
        scaled_renderer.ys * y1,
        scaled_renderer.xs * x2,
        scaled_renderer.ys * y2
    );
}

SDL_Rect SDL_ScaleRect(SDL_ScaledRenderer scaled_renderer, SDL_Rect rect) {
    return (SDL_Rect) {
        (f32)rect.x * scaled_renderer.xs,
        (f32)rect.y * scaled_renderer.ys,
        (f32)rect.w * scaled_renderer.xs,
        (f32)rect.h * scaled_renderer.ys
    };
}

int SDL_ScaledRenderFillRect(SDL_ScaledRenderer scaled_renderer, const SDL_Rect *rect) {
    SDL_Rect scaled_rect = SDL_ScaleRect(scaled_renderer, *rect);
    return SDL_RenderFillRect(scaled_renderer.renderer, &scaled_rect);
}

int SDL_ScaledRenderDrawRect(SDL_ScaledRenderer scaled_renderer, const SDL_Rect *rect) {
    SDL_Rect scaled_rect = SDL_ScaleRect(scaled_renderer, *rect);
    return SDL_RenderDrawRect(scaled_renderer.renderer, &scaled_rect);
}

int SDL_ScaledRenderCopy(
    SDL_ScaledRenderer scaled_renderer,
    SDL_Texture *texture,
    const SDL_Rect *srcrect,
    const SDL_Rect *dstrect
) {
    if (dstrect == NULL) {
        return SDL_RenderCopy(scaled_renderer.renderer, texture, srcrect, dstrect);
    }
    SDL_Rect scaled_dstrect = SDL_ScaleRect(scaled_renderer, *dstrect);
    return SDL_RenderCopy(scaled_renderer.renderer, texture, srcrect, &scaled_dstrect);
}

int SDL_QueryScaledTexture(
    SDL_ScaledRenderer scaled_renderer,
    SDL_Texture *texture,
    u32 *format,
    int *access,
    int *w,
    int *h
) {
    int result = SDL_QueryTexture(texture, format, access, w, h);
    *w = (f32)(*w) / scaled_renderer.xs;
    *h = (f32)(*h) / scaled_renderer.ys;
    return result;
}

TTF_Font *TTF_open_font(SDL_ScaledRenderer scaled_renderer, char *font_file_name, int font_size) {
    int size = (int)(font_size * scaled_renderer.xs);
    return TTF_OpenFont(font_file_name, size);
}
