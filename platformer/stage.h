#ifndef STAGE_H
#define STAGE_H

#include <stdbool.h>
#include <SDL.h>
#include "SDL_utils.h"
#include "input_state.h"
#include "types.h"

typedef struct {
    u64 width, height;
    bool *tiles;
} Stage;


void Stage_init(Stage *stage);
void Stage_destroy(Stage *stage);
void Stage_marshal(const Stage *stage, u8 *buffer);
void Stage_save(const Stage *stage, const char *filename);
void Stage_unmarshal(Stage *stage, const u8 *buffer);
void Stage_load(Stage *stage, const char *filename);
void Stage_draw(Stage *stage, SDL_ScaledRenderer scaled_renderer);
bool *Stage_tile_at(const Stage *stage, i32 x, i32 y);
SDL_Rect Stage_rect_at(const Stage *stage, i32 x, i32 y);
void show_grid(SDL_ScaledRenderer scaled_renderer);

typedef struct {
    f32 x, y;
    f32 dx, dy;
    bool show;
} Player;

void Player_render(Player player, SDL_ScaledRenderer scaled_renderer);
bool Player_collides_above(Player player, const Stage *stage);
bool Player_collides_below(Player player, const Stage *stage);
bool Player_collides_right(Player player, const Stage *stage);
bool Player_collides_left(Player player, const Stage *stage);
void Player_update(Player *player, const Stage *stage, u32 ticks, InputState input_state);

#endif // STAGE_H
