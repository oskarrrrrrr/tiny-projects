#include "SDL_events.h"
#include "SDL_ttf.h"
#include "SDL_video.h"
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

typedef float f32;
typedef double f64;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
typedef int64_t i64;

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define PLAYER_SIZE 20
#define TILE_SIZE 40
#define MIN_LEVEL_WIDTH 32  // 1280 / 40
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
        SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!*window) { _SDL_fail(); }
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) { _SDL_fail(); }

    TTF_Init();
}

void _SDL_destroy(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_DestroyRenderer(*renderer);
    *renderer = NULL;
    SDL_DestroyWindow(*window);
    *window = NULL;
    TTF_Quit();
}

void _SDL_print_rect(SDL_Rect rect) {
    printf("Rect{x: %d, y: %d, w: %d, h: %d}\n", rect.x, rect.y, rect.w, rect.h);
}

void _SDL_get_window_scale(
    SDL_Window *window,
    SDL_Renderer *renderer,
    float *xs,
    float *ys
) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    int rendererWidth, rendererHeight;
    SDL_GetRendererOutputSize(renderer, &rendererWidth, &rendererHeight);
    if (rendererWidth != windowWidth) {
        *xs = ((float)rendererWidth) / windowWidth;
        *ys = ((float)rendererHeight) / windowHeight;
    } else {
        *xs = 1;
        *ys = 1;
    }
}

typedef struct {
    SDL_Renderer *renderer;
    f32 xs, ys;
} ScaledRenderer;

static int _SDL_ScaledRenderDrawLine(ScaledRenderer scaled_renderer, int x1, int y1, int x2, int y2) {
    return SDL_RenderDrawLine(
        scaled_renderer.renderer,
        scaled_renderer.xs * x1,
        scaled_renderer.ys * y1,
        scaled_renderer.xs * x2,
        scaled_renderer.ys * y2
    );
}

static SDL_Rect _SDL_ScaleRect(ScaledRenderer scaled_renderer, SDL_Rect rect) {
    return (SDL_Rect) {
        (float)rect.x * scaled_renderer.xs,
        (float)rect.y * scaled_renderer.ys,
        (float)rect.w * scaled_renderer.xs,
        (float)rect.h * scaled_renderer.ys
    };
}

static int _SDL_ScaledRenderFillRect(ScaledRenderer scaled_renderer, const SDL_Rect *rect) {
    SDL_Rect scaled_rect = _SDL_ScaleRect(scaled_renderer, *rect);
    return SDL_RenderFillRect(scaled_renderer.renderer, &scaled_rect);
}

static int _SDL_ScaledRenderDrawRect(ScaledRenderer scaled_renderer, const SDL_Rect *rect) {
    SDL_Rect scaled_rect = _SDL_ScaleRect(scaled_renderer, *rect);
    return SDL_RenderDrawRect(scaled_renderer.renderer, &scaled_rect);
}

static int _SDL_ScaledRenderCopy(
    ScaledRenderer scaled_renderer,
    SDL_Texture *texture,
    const SDL_Rect *srcrect,
    const SDL_Rect *dstrect
) {
    if (dstrect == NULL) {
        return SDL_RenderCopy(scaled_renderer.renderer, texture, srcrect, dstrect);
    }
    SDL_Rect scaled_dstrect = _SDL_ScaleRect(scaled_renderer, *dstrect);
    return SDL_RenderCopy(scaled_renderer.renderer, texture, srcrect, &scaled_dstrect);
}

static int _SDL_QueryScaledTexture(
    ScaledRenderer scaled_renderer,
    SDL_Texture *texture,
    u32 *format,
    int *access,
    int *w,
    int *h
) {
    int result = SDL_QueryTexture(texture, format, access, w, h);
    *w = (float)(*w) / scaled_renderer.xs;
    *h = (float)(*h) / scaled_renderer.ys;
    return result;
}

typedef struct {
    ScaledRenderer scaled_renderer;
    SDL_Window *window;
    int w, h;
} Window;

TTF_Font *_TTF_open_font(ScaledRenderer scaled_renderer, char *font_file_name, int font_size) {
    int size = (int)(font_size * scaled_renderer.xs);
    return TTF_OpenFont(font_file_name, size);
}

/*** Stage ***/

typedef struct {
    uint64_t width, height;
    bool *tiles;
} Stage;

void Stage_init(Stage *stage) {
    stage->width = MIN_LEVEL_WIDTH;
    stage->height = MIN_LEVEL_HEIGHT;
    stage->tiles = malloc(sizeof(bool) * stage->width * stage->height);
    memset(stage->tiles, 0, sizeof(bool) * stage->width * stage->height);
}

void Stage_destroy(Stage *stage) { free(stage->tiles); }

void Stage_marshal(const Stage *stage, uint8_t *buffer) {
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

void Stage_save(const Stage *stage, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file) {
        size_t buffer_size = sizeof(uint8_t) // version
            + sizeof(stage->width) + sizeof(stage->height) +
            sizeof(bool) * stage->width * stage->height;
        uint8_t *buffer = malloc(buffer_size);
        Stage_marshal(stage, buffer);
        fwrite(buffer, buffer_size, 1, file);
        fclose(file);
        free(buffer);
    } else {
        printf("Failed to open: %s\n", filename);
        exit(1);
    }
}

void Stage_unmarshal(Stage *stage, const uint8_t *buffer) {
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

void Stage_load(Stage *stage, const char *filename) {
    FILE *file = fopen(filename, "rb");
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
    } else {
        printf("Failed to open: %s\n", filename);
        exit(1);
    }
}

void Stage_draw(Stage *stage, ScaledRenderer scaled_renderer) {
    for (size_t r = 0; r < stage->height; r++) {
        for (size_t c = 0; c < stage->width; c++) {
            if (!stage->tiles[r * stage->width + c]) {
                continue;
            }
            SDL_Rect outer_rect = {
                .x = c * TILE_SIZE,
                .y = r * TILE_SIZE,
                .w = TILE_SIZE,
                .h = TILE_SIZE
            };
            SDL_SetRenderDrawColor(scaled_renderer.renderer, 0, 200, 0, 255);
            _SDL_ScaledRenderFillRect(scaled_renderer, &outer_rect);
            SDL_Rect inner_rect = {
                .x = outer_rect.x + 1,
                .y = outer_rect.y + 1,
                .w = outer_rect.w - 2,
                .h = outer_rect.h - 2};
            SDL_SetRenderDrawColor(scaled_renderer.renderer, 0, 128, 0, 255);
            _SDL_ScaledRenderFillRect(scaled_renderer, &inner_rect);
        }
    }
}

bool *Stage_tile_at(const Stage *stage, i32 x, i32 y) {
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

SDL_Rect Stage_rect_at(const Stage *stage, i32 x, i32 y) {
    // TODO: handle invalid inputs?
    size_t row = (float)y / TILE_SIZE;
    size_t col = (float)x / TILE_SIZE;
    return (SDL_Rect) {
        .x = row * TILE_SIZE,
        .y = col * TILE_SIZE,
        .w = TILE_SIZE,
        .h = TILE_SIZE
    };
}

void show_grid(ScaledRenderer scaled_renderer) {
    SDL_SetRenderDrawColor(scaled_renderer.renderer, 210, 70, 148, 255);
    _SDL_ScaledRenderDrawLine(scaled_renderer, 0, 0, 0, SCREEN_HEIGHT);
    for (int x = TILE_SIZE; x <= SCREEN_WIDTH; x += TILE_SIZE) {
        _SDL_ScaledRenderDrawLine(scaled_renderer, x - 1, 0, x - 1, SCREEN_HEIGHT);
        _SDL_ScaledRenderDrawLine(scaled_renderer, x, 0, x, SCREEN_HEIGHT);
    }
    _SDL_ScaledRenderDrawLine(scaled_renderer, 0, 0, SCREEN_WIDTH, 0);
    for (int y = TILE_SIZE; y <= SCREEN_HEIGHT; y += TILE_SIZE) {
        _SDL_ScaledRenderDrawLine(scaled_renderer, 0, y - 1, SCREEN_WIDTH, y - 1);
        _SDL_ScaledRenderDrawLine(scaled_renderer, 0, y, SCREEN_WIDTH, y);
    }
}

typedef struct {
    float x, y;
    float dx, dy;
    bool show;
} Player;

static void Player_render(Player player, ScaledRenderer scaled_renderer) {
    if (!player.show) { return; }
    SDL_SetRenderDrawColor(scaled_renderer.renderer, 0, 128, 0, 255);
    SDL_Rect rect = {
        .x = player.x,
        .y = player.y + 1,
        .w = PLAYER_SIZE,
        .h = PLAYER_SIZE
    };
    _SDL_ScaledRenderFillRect(scaled_renderer, &rect);
}

static bool Player_collides_above(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x + 2), roundf(player.y))
        || *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 2), roundf(player.y))
    );
}

static bool Player_collides_below(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x + 2), roundf(player.y + PLAYER_SIZE))
        || *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 2), roundf(player.y + PLAYER_SIZE))
    );
}

static bool Player_collides_right(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 1), roundf(player.y + 1))
        || *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 1), roundf(player.y + PLAYER_SIZE - 1))
    );
}

static bool Player_collides_left(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x - 1), roundf(player.y + 1))
        || *Stage_tile_at(stage, roundf(player.x - 1), roundf(player.y + PLAYER_SIZE - 1))
    );
}

typedef struct {
    Window window;
    char *stage_name;
    Stage *stage;
    Player player;
    bool show_grid;
} App;

App App_new() {
    SDL_Window *window;
    SDL_Renderer *renderer;
    _SDL_init(&window, &renderer);
    f32 xs, ys;
    _SDL_get_window_scale(window, renderer, &xs, &ys);
    return (App){
        .window = {
            .scaled_renderer = {
                .renderer = renderer,
                .xs = xs,
                .ys = ys
            },
            .window = window,
            .w = SCREEN_WIDTH,
            .h = SCREEN_HEIGHT
        },
        .player = {
            .x = 0,
            .y = 0,
            .dx = 0,
            .dy = 0,
            .show = false
        },
        .stage_name = NULL,
        .stage = NULL,
        .show_grid = false
    };
}

void App_destroy(App app) {
    _SDL_destroy(&app.window.window, &app.window.scaled_renderer.renderer);
    if (app.stage != NULL) {
        Stage_destroy(app.stage);
        free(app.stage);
    }
}

void App_load_stage(App *app, char *stage_file) {
    app->stage_name = stage_file;
    app->stage = malloc(sizeof(Stage));
    Stage_load(app->stage, app->stage_name);
}

void App_show_file_name(App app) {
    TTF_Font *font = _TTF_open_font(
        app.window.scaled_renderer, "assets/Lato/Lato-Regular.ttf", 16
    );
    if (font == NULL) { _SDL_fail(); }
    SDL_Color gray = {64, 64, 64, 255};
    SDL_Surface *font_surface = TTF_RenderUTF8_Blended(font, app.stage_name, gray);
    if (font_surface == NULL) { _SDL_fail(); }
    SDL_Texture *font_texture = SDL_CreateTextureFromSurface(
        app.window.scaled_renderer.renderer, font_surface
    );
    int w, h;
    _SDL_QueryScaledTexture(app.window.scaled_renderer, font_texture, NULL, NULL, &w, &h);
    int x_margin = 5, y_margin = 2;
    SDL_Rect dst = {SCREEN_WIDTH - w - x_margin, SCREEN_HEIGHT - h - y_margin, w, h};
    _SDL_ScaledRenderCopy(app.window.scaled_renderer, font_texture, NULL, &dst);
    SDL_DestroyTexture(font_texture);
    TTF_CloseFont(font);
}

void App_render(App app) {
    SDL_SetRenderDrawColor(app.window.scaled_renderer.renderer, 128, 128, 128, 255);
    SDL_RenderClear(app.window.scaled_renderer.renderer);
    Stage_draw(app.stage, app.window.scaled_renderer);
    if (app.show_grid) {
        show_grid(app.window.scaled_renderer);
    }
    Player_render(app.player, app.window.scaled_renderer);
    App_show_file_name(app);
    SDL_RenderPresent(app.window.scaled_renderer.renderer);
}

typedef enum {
    TOOL_PLAYER_PLACER,
    TOOL_TILE_MODIFIER,
    TOOL_COUNT,
} ToolType;

typedef enum {
    TILE_MODIFIER_TOOL_MODE_DELETE = 0,
    TILE_MODIFIER_TOOL_MODE_ADD = 1
} TileModifierToolMode;

typedef struct {
    ToolType type;
    TileModifierToolMode mode;
} TileModifier;

typedef struct {
    ToolType type;
} PlayerPlacer;

typedef union {
    ToolType type;
    TileModifier tile_modifier;
    PlayerPlacer player_placer;
} Tool;

typedef struct {
    bool mouse_down;
    bool left_down;
    bool right_down;
    bool space_down;
} InputState;

int main() {
    App app = App_new();
    /* app.stage_name = "stages/test_stage_2.bin"; */
    /* app.stage = malloc(sizeof(Stage)); */
    /* Stage_init(app.stage); */
    App_load_stage(&app, "stages/test_stage_2.bin");

    bool continue_ = true;

    Tool tool;
    tool.type = TOOL_TILE_MODIFIER;

    InputState input_state;
    u32 last_ticks = SDL_GetTicks();

    while (continue_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    continue_ = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    input_state.mouse_down = true;
                    switch (tool.type) {
                    case TOOL_TILE_MODIFIER: {
                        bool *tile = Stage_tile_at(app.stage, event.motion.x, event.motion.y);
                        if (tile != NULL) {
                            *tile = !(*tile);
                            tool.tile_modifier.mode = *tile;
                        }
                        break;
                    }
                    case TOOL_PLAYER_PLACER:
                        app.player.show = true;
                        app.player.x = event.motion.x;
                        app.player.y = event.motion.y;
                        break;
                    case TOOL_COUNT: break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    input_state.mouse_down = false;
                    break;
                case SDL_MOUSEMOTION:
                    if (input_state.mouse_down) {
                        switch (tool.type) {
                        case TOOL_TILE_MODIFIER: {
                            bool *tile = Stage_tile_at(app.stage, event.motion.x, event.motion.y);
                            if (tile != NULL) {
                                *tile = tool.tile_modifier.mode;
                            }
                            break;
                        }
                        case TOOL_PLAYER_PLACER:
                            break;
                        case TOOL_COUNT: break;
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
                        app.show_grid = !app.show_grid;
                        break;
                    case SDL_SCANCODE_S:
                        Stage_save(app.stage, app.stage_name);
                        printf("Saved to %s\n", app.stage_name);
                        break;
                    case SDL_SCANCODE_T:
                        tool.type = (tool.type + 1) % TOOL_COUNT;
                        switch (tool.type) {
                        case TOOL_TILE_MODIFIER:
                            tool.tile_modifier.mode = TILE_MODIFIER_TOOL_MODE_ADD;
                        case TOOL_PLAYER_PLACER: break;
                        case TOOL_COUNT: break;
                        }
                        break;
                    case SDL_SCANCODE_SPACE:
                        input_state.space_down = true;
                        break;
                    case SDL_SCANCODE_LEFT:
                        input_state.left_down = true;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        input_state.right_down = true;
                        break;
                    default: break;
                    }
                    break;
                case SDL_KEYUP:
                    if (event.key.repeat != 0) { break; }
                    switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_LEFT:
                        input_state.left_down = false;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        input_state.right_down = false;
                        break;
                    case SDL_SCANCODE_SPACE:
                        input_state.space_down = false;
                        break;
                    default: break;
                    }
                    break;
            }
        }
        u32 curr_ticks = SDL_GetTicks();
        u32 ticks_diff = curr_ticks - last_ticks;
        float MAX_DY = 0.5;
        float SIDE_MOVEMENT_SPEED = 0.4;
        float GRAVITY = 0.004;
        if (app.player.show) {
            if (input_state.space_down && Player_collides_below(app.player, app.stage)) {
                app.player.dy = fmax(app.player.dy - 1, -1.2);
            }
            for (u32 i = 0; i < ticks_diff; i++) {
                if (!Player_collides_below(app.player, app.stage)) {
                    app.player.dy += GRAVITY;
                }
                if (app.player.dy > MAX_DY) {
                    app.player.dy = MAX_DY;
                }
                app.player.y += app.player.dy;
                if (app.player.y > SCREEN_HEIGHT) { app.player.dy = 0; }
                if (app.player.y >= 0 && app.player.y + PLAYER_SIZE <= SCREEN_HEIGHT) {
                    if (Player_collides_below(app.player, app.stage)) {
                        app.player.dy = 0;
                    }
                    if (Player_collides_above(app.player, app.stage)) {
                        app.player.dy = 0;
                    }
                }
                if (input_state.left_down && !Player_collides_left(app.player, app.stage)) {
                    app.player.dx = fmax(app.player.dx - SIDE_MOVEMENT_SPEED, -SIDE_MOVEMENT_SPEED);
                } else if (input_state.right_down && !Player_collides_right(app.player, app.stage)) {
                    app.player.dx = fmin(app.player.dx + SIDE_MOVEMENT_SPEED, SIDE_MOVEMENT_SPEED);
                } else {
                    app.player.dx = 0;
                }
                app.player.x += app.player.dx;
            }
        }
        if (ticks_diff > 0) {
            last_ticks = curr_ticks;
            App_render(app);
        }
        SDL_Delay(5);
    }
    App_destroy(app);
    return 0;
}
