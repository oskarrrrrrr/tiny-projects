#include <SDL.h>
#include <SDL_events.h>
#include <SDL_ttf.h>
#include <SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <dirent.h>

#include "SDL_utils.h"
#include "stage.h"
#include "types.h"
#include "input_state.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define PLAYER_SIZE 20
#define TILE_SIZE 40
#define MIN_LEVEL_WIDTH 32  // 1280 / 40
#define MIN_LEVEL_HEIGHT 18 // 720 / 40

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
    SDL_init(&window, &renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    f32 xs, ys;
    SDL_get_window_scale(window, renderer, &xs, &ys);
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
    SDL_destroy(&app.window.window, &app.window.scaled_renderer.renderer);
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
    TTF_Font *font = TTF_open_font(
        app.window.scaled_renderer, "assets/Lato/Lato-Regular.ttf", 16
    );
    if (font == NULL) { SDL_fail(); }
    SDL_Color gray = {64, 64, 64, 255};
    SDL_Surface *font_surface = TTF_RenderUTF8_Blended(font, app.stage_name, gray);
    if (font_surface == NULL) { SDL_fail(); }
    SDL_Texture *font_texture = SDL_CreateTextureFromSurface(
        app.window.scaled_renderer.renderer, font_surface
    );
    int w, h;
    SDL_QueryScaledTexture(app.window.scaled_renderer, font_texture, NULL, NULL, &w, &h);
    int x_margin = 5, y_margin = 2;
    SDL_Rect dst = {SCREEN_WIDTH - w - x_margin, SCREEN_HEIGHT - h - y_margin, w, h};
    SDL_ScaledRenderCopy(app.window.scaled_renderer, font_texture, NULL, &dst);
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


int main() {
    struct dirent *entry;
    DIR *dp = opendir("stages");
    while((entry = readdir(dp)))
        puts(entry->d_name);
    closedir(dp);

    App app = App_new();
    /* app.stage_name = "stages/test_stage_2.bin"; */
    /* app.stage = malloc(sizeof(Stage)); */
    /* Stage_init(app.stage); */
    App_load_stage(&app, "stages/test_stage.bin");

    Tool tool;
    tool.type = TOOL_TILE_MODIFIER;

    InputState input_state;
    u32 last_ticks = SDL_GetTicks();

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: goto quit;
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
                    case SDL_SCANCODE_Q: goto quit;
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
        Player_update(&app.player, app.stage, ticks_diff, input_state);

        if (ticks_diff > 0) {
            last_ticks = curr_ticks;
            App_render(app);
        }
        SDL_Delay(5);
    }

quit:
    App_destroy(app);
    return 0;
}
