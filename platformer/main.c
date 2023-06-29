#include "SDL_events.h"
#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TILE_SIZE 40
#define MIN_LEVEL_WIDTH 32 // 1280 / 40
#define MIN_LEVEL_HEIGHT 18 // 720 / 40

/*** SDL Utilities ***/

void _SDL_fail() {
    printf("SDL ERROR: %s\n", SDL_GetError());
    exit(1);
}

void _SDL_init(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { _SDL_fail(); }
    *window = SDL_CreateWindow(
        "Platformer",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        0 // TODO: do we need SDL_WINDOW_ALLOW_HIGHDPI?
    );
    if (!*window) { _SDL_fail(); }
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) { _SDL_fail(); }
}

void _SDL_destroy(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_DestroyRenderer(*renderer);
    *renderer = NULL;
    SDL_DestroyWindow(*window);
    *window = NULL;
}

void _SDL_print_rect(SDL_Rect rect) {
    printf("Rect{x: %d, y: %d, w: %d, h: %d}\n", rect.x, rect.y, rect.w, rect.h);
}

/*** Stage ***/

typedef struct {
    uint64_t width, height;
    bool *tiles;
} Stage;

void Stage_init(Stage* stage) {
    stage->width = MIN_LEVEL_WIDTH;
    stage->height = MIN_LEVEL_HEIGHT;
    stage->tiles = malloc(sizeof(bool) * stage->width * stage->height);
    memset(stage->tiles, 0, sizeof(bool) * stage->width * stage->height);
}

void Stage_marshal(const Stage* stage, uint8_t* buffer) {
    // version
    uint8_t version = 1;
    memcpy(buffer, &version, sizeof(version));
    buffer += sizeof(version);
    // width
    memcpy(buffer, &stage->width, sizeof(stage->width));
    buffer += sizeof(stage->width);
    // height
    memcpy(buffer, &stage->height, sizeof(stage->height));
    buffer += sizeof(stage->height);
    // tiles
    memcpy(buffer, stage->tiles, sizeof(bool) * stage->width * stage->height);
    buffer += sizeof(bool) * stage->width * stage->height;
}

void Stage_save(const Stage* stage, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file) {
        size_t buffer_size =
            sizeof(uint8_t) // version
            + sizeof(stage->width)
            + sizeof(stage->height)
            + sizeof(bool) * stage->width * stage->height;
        uint8_t* buffer = malloc(buffer_size);
        Stage_marshal(stage, buffer);
        fwrite(buffer, buffer_size, 1, file);
        fclose(file);
        free(buffer);
    } else {
        printf("Failed to open: %s\n", filename);
        exit(1);
    }
}

void Stage_unmarshal(Stage* stage, const uint8_t* buffer) {
    // version
    uint8_t version;
    memcpy(&version, buffer, sizeof(version));
    if (version != 1) {
        printf("Expected version to be equal to 1, got: %x\n", version);
        exit(1);
    }
    buffer += sizeof(version);
    // width
    memcpy(&stage->width, buffer, sizeof(stage->width));
    buffer += sizeof(stage->width);
    // height
    memcpy(&stage->height, buffer, sizeof(stage->height));
    buffer += sizeof(stage->height);
    // tiles
    size_t tiles_size = sizeof(bool) * stage->width * stage->height;
    memcpy(stage->tiles, buffer, tiles_size);
    buffer += tiles_size;
}

void Stage_load(Stage* stage, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file) {
        // peek size
        uint8_t peek_buffer[sizeof(uint8_t) + 2 * sizeof(size_t)];
        fread(peek_buffer, sizeof(peek_buffer), 1, file);
        rewind(file);
        uint8_t *buff = peek_buffer;
        uint8_t version;
        memcpy(&version, buff, sizeof(version));
        if (version != 1) {
            printf("Expected version to be equal to 1, got: %x\n", version);
            exit(1);
        }
        buff += sizeof(version);
        size_t width, height;
        // width
        memcpy(&width, buff, sizeof(stage->width));
        buff += sizeof(width);
        // height
        memcpy(&height, buff, sizeof(stage->height));
        buff += sizeof(height);

        size_t tiles_size = sizeof(bool) * width * height;
        stage->tiles = malloc(sizeof(bool) * width * height);

        size_t buffer_size = sizeof(peek_buffer) + tiles_size;
        uint8_t *buffer = malloc(buffer_size);
        fread(buffer, buffer_size, 1, file);

        Stage_unmarshal(stage, buffer);

        free(buffer);
        fclose(file);
    } else  {
        printf("Failed to open: %s\n", filename);
        exit(1);
    }
}

void Stage_draw(Stage* stage, SDL_Renderer *renderer) {
    for (size_t r = 0; r < stage->height; r++) {
        for (size_t c = 0; c < stage->width; c++) {
            if (!stage->tiles[r * stage->width + c]) { continue; }
            SDL_Rect outer_rect = {
                .x = c * TILE_SIZE,
                .y = r * TILE_SIZE,
                .w = TILE_SIZE,
                .h = TILE_SIZE
            };
            SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
            SDL_RenderFillRect(renderer, &outer_rect);
            SDL_Rect inner_rect = {
                .x =  outer_rect.x+1,
                .y =  outer_rect.y+1,
                .w =  outer_rect.w-2,
                .h =  outer_rect.h-2
            };
            SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
            SDL_RenderFillRect(renderer, &inner_rect);
        }
    }
}

bool* Stage_tile_at(Stage* stage, int32_t x, int32_t y) {
    if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT) {
        return NULL;
    }
    size_t row = (float)y / TILE_SIZE;
    size_t col = (float)x / TILE_SIZE;
    if (row > stage->height || col > stage->width) {
        return NULL;
    }
    return stage->tiles + (row * stage->width + col);
}

void show_grid(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 210, 70, 148, 255);
    SDL_RenderDrawLine(renderer, 0, 0, 0, SCREEN_HEIGHT);
    for (int x = TILE_SIZE; x <= SCREEN_WIDTH; x += TILE_SIZE) {
        SDL_RenderDrawLine(renderer, x-1, 0, x-1, SCREEN_HEIGHT);
        SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_HEIGHT);
    }
    SDL_RenderDrawLine(renderer, 0, 0, SCREEN_WIDTH, 0);
    for (int y = TILE_SIZE; y <= SCREEN_HEIGHT; y += TILE_SIZE) {
        SDL_RenderDrawLine(renderer, 0, y-1, SCREEN_WIDTH, y-1);
        SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
}

int main() {
    SDL_Window *window;
    SDL_Renderer *renderer;
    _SDL_init(&window, &renderer);

    char *test_stage_path = "stages/test_stage.bin";

    Stage stage;
    Stage_load(&stage, test_stage_path);

    bool continue_ = true;
    bool show_grid_ = false;

    bool mouse_down = false;
    bool brush_mode = false;

    while(continue_) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    continue_ = false;
                case SDL_MOUSEBUTTONDOWN:
                    mouse_down = true;
                    bool* tile = Stage_tile_at(&stage, event.motion.x, event.motion.y);
                    if (tile != NULL)  {
                        *tile = !(*tile);
                        brush_mode = *tile;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    mouse_down = false;
                    break;
                case SDL_MOUSEMOTION:
                    if (mouse_down) {
                        bool* tile = Stage_tile_at(&stage, event.motion.x, event.motion.y);
                        if (tile != NULL) {
                            *tile = brush_mode;
                        }
                    }
                    break;
                case SDL_KEYDOWN:
                    if (event.key.repeat != 0) { break; }
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_Q:
                            continue_ = false;
                            break;
                        case SDL_SCANCODE_G:
                            show_grid_ = !show_grid_;
                            break;
                        case SDL_SCANCODE_S:
                            Stage_save(&stage, test_stage_path);
                            printf("Saved to %s\n", test_stage_path);
                            break;
                        default:
                            break;
                    }
            }
        }

        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);
        Stage_draw(&stage, renderer);
        if (show_grid_) { show_grid(renderer); }
        SDL_RenderPresent(renderer);
    }
    free(stage.tiles);
    _SDL_destroy(&window, &renderer);
    return 0;
}
