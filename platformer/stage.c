#include <stdlib.h>
#include <string.h>
#include "stage.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define PLAYER_SIZE 20
#define TILE_SIZE 40
#define MIN_LEVEL_WIDTH 32  // 1280 / 40
#define MIN_LEVEL_HEIGHT 18 // 720 / 40

#define MAX_DY 0.5
#define SIDE_MOVEMENT_SPEED 0.4
#define GRAVITY 0.004

void Stage_init(Stage *stage) {
    stage->width = MIN_LEVEL_WIDTH;
    stage->height = MIN_LEVEL_HEIGHT;
    stage->tiles = malloc(sizeof(bool) * stage->width * stage->height);
    memset(stage->tiles, 0, sizeof(bool) * stage->width * stage->height);
}

void Stage_destroy(Stage *stage) {
    free(stage->tiles);
}

void Stage_marshal(const Stage *stage, u8 *buffer) {
    // version
    u8 version = 1;
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
        size_t buffer_size = sizeof(u8) // version
            + sizeof(stage->width) + sizeof(stage->height) +
            sizeof(bool) * stage->width * stage->height;
        u8 *buffer = malloc(buffer_size);
        Stage_marshal(stage, buffer);
        fwrite(buffer, buffer_size, 1, file);
        fclose(file);
        free(buffer);
    } else {
        printf("Failed to open: %s\n", filename);
        exit(1);
    }
}

void Stage_unmarshal(Stage *stage, const u8 *buffer) {
    // version
    u8 version;
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
        u8 peek_buffer[sizeof(u8) + 2 * sizeof(size_t)];
        fread(peek_buffer, sizeof(peek_buffer), 1, file);
        rewind(file);
        u8 *buff = peek_buffer;
        u8 version;
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
        u8 *buffer = malloc(buffer_size);
        fread(buffer, buffer_size, 1, file);

        Stage_unmarshal(stage, buffer);

        free(buffer);
        fclose(file);
    } else {
        printf("Failed to open: %s\n", filename);
        exit(1);
    }
}

void Stage_draw(Stage *stage, SDL_ScaledRenderer scaled_renderer) {
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
            SDL_ScaledRenderFillRect(scaled_renderer, &outer_rect);
            SDL_Rect inner_rect = {
                .x = outer_rect.x + 1,
                .y = outer_rect.y + 1,
                .w = outer_rect.w - 2,
                .h = outer_rect.h - 2};
            SDL_SetRenderDrawColor(scaled_renderer.renderer, 0, 128, 0, 255);
            SDL_ScaledRenderFillRect(scaled_renderer, &inner_rect);
        }
    }
}

bool *Stage_tile_at(const Stage *stage, i32 x, i32 y) {
    if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT) {
        return NULL;
    }
    size_t row = (f32)y / TILE_SIZE;
    size_t col = (f32)x / TILE_SIZE;
    if (row > stage->height || col > stage->width) {
        return NULL;
    }
    return stage->tiles + (row * stage->width + col);
}

SDL_Rect Stage_rect_at(const Stage *stage, i32 x, i32 y) {
    size_t row = (f32)y / TILE_SIZE;
    size_t col = (f32)x / TILE_SIZE;
    return (SDL_Rect) {
        .x = row * TILE_SIZE,
        .y = col * TILE_SIZE,
        .w = TILE_SIZE,
        .h = TILE_SIZE
    };
}

void show_grid(SDL_ScaledRenderer scaled_renderer) {
    SDL_SetRenderDrawColor(scaled_renderer.renderer, 210, 70, 148, 255);
    SDL_ScaledRenderDrawLine(scaled_renderer, 0, 0, 0, SCREEN_HEIGHT);
    for (int x = TILE_SIZE; x <= SCREEN_WIDTH; x += TILE_SIZE) {
        SDL_ScaledRenderDrawLine(scaled_renderer, x - 1, 0, x - 1, SCREEN_HEIGHT);
        SDL_ScaledRenderDrawLine(scaled_renderer, x, 0, x, SCREEN_HEIGHT);
    }
    SDL_ScaledRenderDrawLine(scaled_renderer, 0, 0, SCREEN_WIDTH, 0);
    for (int y = TILE_SIZE; y <= SCREEN_HEIGHT; y += TILE_SIZE) {
        SDL_ScaledRenderDrawLine(scaled_renderer, 0, y - 1, SCREEN_WIDTH, y - 1);
        SDL_ScaledRenderDrawLine(scaled_renderer, 0, y, SCREEN_WIDTH, y);
    }
}

// Player

void Player_render(Player player, SDL_ScaledRenderer scaled_renderer) {
    if (!player.show) { return; }
    SDL_SetRenderDrawColor(scaled_renderer.renderer, 0, 128, 0, 255);
    SDL_Rect rect = {
        .x = player.x,
        .y = player.y + 1,
        .w = PLAYER_SIZE,
        .h = PLAYER_SIZE
    };
    SDL_ScaledRenderFillRect(scaled_renderer, &rect);
}

bool Player_collides_above(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x + 2), roundf(player.y))
        || *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 2), roundf(player.y))
    );
}

bool Player_collides_below(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x + 2), roundf(player.y + PLAYER_SIZE))
        || *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 2), roundf(player.y + PLAYER_SIZE))
    );
}

bool Player_collides_right(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 1), roundf(player.y + 1))
        || *Stage_tile_at(stage, roundf(player.x + PLAYER_SIZE - 1), roundf(player.y + PLAYER_SIZE - 1))
    );
}

bool Player_collides_left(Player player, const Stage *stage) {
    return (
        *Stage_tile_at(stage, roundf(player.x - 1), roundf(player.y + 1))
        || *Stage_tile_at(stage, roundf(player.x - 1), roundf(player.y + PLAYER_SIZE - 1))
    );
}

void Player_update(Player *player, const Stage *stage, u32 ticks, InputState input_state) {
    if (player->show) {
        if (input_state.space_down && Player_collides_below(*player, stage)) {
            player->dy = fmax(player->dy - 1, -1.2);
        }
        for (u32 i = 0; i < ticks; i++) {
            if (!Player_collides_below(*player, stage)) {
                player->dy += GRAVITY;
            }
            if (player->dy > MAX_DY) {
                player->dy = MAX_DY;
            }
            player->y += player->dy;
            if (player->y > SCREEN_HEIGHT) { player->dy = 0; }
            if (player->y >= 0 && player->y + PLAYER_SIZE <= SCREEN_HEIGHT) {
                if (Player_collides_below(*player, stage)) {
                    player->dy = 0;
                }
                if (Player_collides_above(*player, stage)) {
                    player->dy = 0;
                }
            }
            if (input_state.left_down && !Player_collides_left(*player, stage)) {
                player->dx = fmax(player->dx - SIDE_MOVEMENT_SPEED, -SIDE_MOVEMENT_SPEED);
            } else if (input_state.right_down && !Player_collides_right(*player, stage)) {
                player->dx = fmin(player->dx + SIDE_MOVEMENT_SPEED, SIDE_MOVEMENT_SPEED);
            } else {
                player->dx = 0;
            }
            player->x += player->dx;
        }
    }
}
