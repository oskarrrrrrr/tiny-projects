#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdlib.h>

#define DEBUG 0

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 960

#define SPACESHIP_WIDTH 55
#define SPACESHIP_HEIGHT (SPACESHIP_WIDTH * 1.05)
#define SPEED 2

#define BULLET_DAMAGE 35
#define BULLET_SPEED 10
#define BULLET_WIDTH 8
#define BULLET_HEIGHT (BULLET_WIDTH * 3.3)
#define MAX_BULLETS_NUM 50
#define RELOAD_TIME 20
#define PLAYER_HEALTH 100

#define SPAWN_DELAY 30
#define MAX_SPAWN 1
#define ENEMIES_COUNT 8
#define ENEMY_HEALTH 100

#define MAX_FPS 60

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

void sdl_fail() {
    printf("SDL ERROR: %s\n", SDL_GetError());
    exit(1);
}

SDL_Texture *sdl_load_texture(SDL_Renderer *renderer, char *file_name) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, file_name);
    if (texture == NULL) { sdl_fail(); }
    return texture;
}

void sdl_render_copy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect) {
    SDL_RenderCopy(renderer, texture, srcrect, dstrect);
}

typedef struct {
    double rotation; // rotation in degrees
    float dx, dy;
    SDL_Rect rect;
    SDL_Texture *texture;
} Entity;

void Entity_new_fill(
    Entity *entity, double rotation, float dx, float dy, SDL_Rect rect, SDL_Texture *texture
) {
    entity->rotation = rotation;
    entity->dx = dx;
    entity->dy = dy;
    entity->rect = rect;
    entity->texture = texture;
}

Entity *Entity_new(double rotation, float dx, float dy, SDL_Rect rect, SDL_Texture *texture) {
    Entity *entity = malloc(sizeof(Entity));
    Entity_new_fill(entity, rotation, dx, dy, rect, texture);
    return entity;
}

void Entity_destroy(Entity *entity) {
    free(entity);
}

void Entity_move(Entity *entity) {
    entity->rect.x += entity->dx;
    entity->rect.y += entity->dy;
}

void Entity_render(Entity *entity, SDL_Renderer *renderer) {
    SDL_RenderCopyEx(
        renderer,
        entity->texture,
        NULL,
        &entity->rect,
        entity->rotation,
        NULL,
        SDL_FLIP_NONE
    );
    if(DEBUG) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128);
        SDL_RenderDrawRect(renderer, &entity->rect);
    }
}

typedef Entity Bullet;

void Bullet_new_fill(
    Bullet *bullet, double rotation, float dx, float dy, SDL_Rect rect, SDL_Texture *texture
) {
    Entity_new_fill(bullet, rotation, dx, dy, rect, texture);
}

void Bullet_new_fill_regular_bullet(Bullet *bullet, SDL_Renderer *renderer, Uint32 x, Uint32 y, bool reverse) {
    static SDL_Texture *texture = NULL;
    if (texture == NULL) {
        texture = sdl_load_texture(renderer, "assets/laser.png");
    }
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = BULLET_WIDTH;
    rect.h = BULLET_HEIGHT;
    float bullet_speed = -BULLET_SPEED;
    if (reverse) { bullet_speed *= -1; }
    Bullet_new_fill(bullet, 0, 0, bullet_speed, rect, texture);
}

void Bullet_destroy(Bullet *bullet) {
    free(bullet);
}

typedef struct {
    Bullet objs[MAX_BULLETS_NUM];
    bool used[MAX_BULLETS_NUM];
    size_t head;
    size_t tail;
} BulletsManager;

BulletsManager *BulletsManager_new() {
    BulletsManager *bullets_manager = malloc(sizeof(BulletsManager));
    memset(bullets_manager->objs, 0, sizeof(Bullet*) * MAX_BULLETS_NUM);
    memset(bullets_manager->used, 0, sizeof(bool) * MAX_BULLETS_NUM);
    bullets_manager->head = 0;
    bullets_manager->tail = 0;
    return bullets_manager;
}

void BulletsManager_destroy(BulletsManager *bullets_manager) {
    free(bullets_manager);
}

void BulletsManager_add_bullet(
    BulletsManager *bullets, SDL_Renderer *renderer, SDL_Rect *spaceship_rect, bool reverse
) {
    if (reverse) {
        Bullet_new_fill_regular_bullet(
            &bullets->objs[bullets->tail],
            renderer,
            spaceship_rect->x + (spaceship_rect->w / 2.) - (BULLET_WIDTH / 2.),
            spaceship_rect->y + spaceship_rect->h,
            reverse
        );
    } else {
        Bullet_new_fill_regular_bullet(
            &bullets->objs[bullets->tail],
            renderer,
            spaceship_rect->x + (spaceship_rect->w / 2.) - (BULLET_WIDTH / 2.),
            spaceship_rect->y - BULLET_HEIGHT,
            reverse
        );
    }
    bullets->used[bullets->tail] = true;
    bullets->tail = (bullets->tail + 1) % MAX_BULLETS_NUM;
    if (bullets->tail == bullets->head) {
        bullets->head = (bullets->head + 1) % MAX_BULLETS_NUM;
    }
}

void BulletsManager_move_bullets(BulletsManager *bullets_manager) {
    for (
        size_t i = bullets_manager->head;
        i != bullets_manager->tail && !bullets_manager->used[i];
        i = (i + 1) % MAX_BULLETS_NUM
    ) {
        bullets_manager->head = (bullets_manager->head + 1) % MAX_BULLETS_NUM;
    }

    for (
        size_t i = bullets_manager->head;
        i != bullets_manager->tail;
        i = (i + 1) % MAX_BULLETS_NUM
    ) {
        Bullet *b = &bullets_manager->objs[i];
        if (bullets_manager->used[i]) {
            Entity_move(b);
            if (b->rect.y + BULLET_HEIGHT < 0) {
                bullets_manager->used[i] = false;
            }
        }
    }
}

void BulletsManager_render_bullets(BulletsManager *bullets, SDL_Renderer *renderer) {
    for (size_t i = bullets->head; i != bullets->tail; i = (i + 1) % MAX_BULLETS_NUM) {
        Bullet *b = &bullets->objs[i];
        if (bullets->used[i]) {
            Entity_render(b, renderer);
        }
    }
}

typedef struct Spaceship {
    Entity *entity;
    Uint32 reload;  // frames left for reloading, can shoot whenever 0
    bool fire;  // player wants to shoot
    Uint32 max_health;
    Uint32 health;
    struct Spaceship *next;
} Spaceship;

Spaceship *Spaceship_new(Entity *entity, Uint32 health) {
    Spaceship *ss = malloc(sizeof(Spaceship));
    ss->entity = entity;
    ss->fire = false;
    ss->reload = 0;
    ss->next = NULL;
    ss->max_health = ss->health = health;
    return ss;
}

Spaceship *Spaceship_new_player_spaceship(SDL_Renderer *renderer) {
    SDL_Texture *texture = sdl_load_texture(renderer, "assets/spaceship.png");
    SDL_Rect rect;
    rect.x = (SCREEN_WIDTH - SPACESHIP_WIDTH) / 2;
    rect.y = SCREEN_HEIGHT / 5. * 4;
    rect.w = SPACESHIP_WIDTH;
    rect.h = SPACESHIP_HEIGHT;
    Entity *entity = Entity_new(0, 0, 0, rect, texture);
    return Spaceship_new(entity, PLAYER_HEALTH);
}

void Spaceship_destroy(Spaceship *spaceship) {
    Entity_destroy(spaceship->entity);
    free(spaceship);
}

void Spaceship_decrease_reload_counter(Spaceship *spaceship) {
    if (spaceship->reload > 0) {
        spaceship->reload -= 1;
    }
}

/* Call this routine every tick/frame */
void Spaceship_fire(Spaceship *spaceship, BulletsManager *bullets, SDL_Renderer *renderer, bool reverse) {
    Spaceship_decrease_reload_counter(spaceship);
    if (spaceship->fire && spaceship->reload == 0) {
        BulletsManager_add_bullet(bullets, renderer, &spaceship->entity->rect, reverse);
        spaceship->reload = RELOAD_TIME;
    }
}

void Spaceship_render_healthbar(Spaceship *spaceship, SDL_Renderer *renderer) {
    SDL_Rect sr = spaceship->entity->rect;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect healthbar_empty;
    healthbar_empty.x = sr.x + (sr.w * 0.05);
    if (spaceship->entity->rotation == 0) {
        healthbar_empty.y = sr.y + sr.h + 10;
    } else {
        healthbar_empty.y = sr.y - 10;
    }
    healthbar_empty.w = sr.w * 0.9;
    healthbar_empty.h = 6;
    SDL_RenderDrawRect(renderer, &healthbar_empty);

    if (spaceship->health > 50) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else if (spaceship->health > 30) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }

    SDL_Rect healthbar;
    healthbar_empty.x = sr.x + (sr.w * 0.05) + 1;
    if(spaceship->entity->rotation == 0) {
        healthbar_empty.y = sr.y + sr.h + 10 + 1;
    } else {
        healthbar_empty.y = sr.y - 10 + 1;
    }
    healthbar_empty.w = sr.w * 0.9 * (((double)spaceship->health) / spaceship->max_health) - 2;
    healthbar_empty.h = 4;
    SDL_RenderFillRect(renderer, &healthbar_empty);
}

void Spaceship_render(Spaceship *spaceship, SDL_Renderer *renderer) {
    Entity_render(spaceship->entity, renderer);
    Spaceship_render_healthbar(spaceship, renderer);
}

char *Rect_to_str(SDL_Rect rect) {
    char *result = calloc(100, sizeof(char));
    sprintf(result, "(x, y): (%d, %d), (w, h): (%d, %d)", rect.x, rect.y, rect.w, rect.h);
    return result;
}

void apply_bullet_hits(Spaceship *spaceship, BulletsManager *bullets_manager) {
    Spaceship *prev = NULL, *curr = spaceship;
    while (curr != NULL) {
        for (
            size_t i = bullets_manager->head;
            i != bullets_manager->tail;
            i = (i + 1) % MAX_BULLETS_NUM
        ) {
            if (!bullets_manager->used[i]) {
                continue;
            }
            SDL_Rect *ship_rect = &curr->entity->rect;
            SDL_Rect *bullet_rect = &bullets_manager->objs[i].rect;
            if (SDL_HasIntersection(ship_rect, bullet_rect)) {
                Uint32 bullet_damage;
                if (rand() % 2) {
                    bullet_damage = BULLET_DAMAGE + (rand() % BULLET_DAMAGE / 3);
                } else {
                    bullet_damage = BULLET_DAMAGE - (rand() % BULLET_DAMAGE / 3);
                }
                if (curr->health > bullet_damage) {
                    curr->health -= bullet_damage;
                } else {
                    curr->health = 0;
                }
                if (curr->health == 0) {
                    if (prev == NULL) {
                        return;
                    } else {
                        prev->next = curr->next;
                        Spaceship_destroy(curr);
                        curr = prev->next;
                        if (curr == NULL) {
                            break;
                        }
                    }
                }
                bullets_manager->used[i] = false;
            }
        }
        prev = curr;
        if (curr != NULL) {
            curr = curr->next;
        }
    }
}

void spawn_enemies(Spaceship *spaceship, SDL_Renderer *renderer) {
    Spaceship *curr = spaceship->next, *last = spaceship;
    Uint32 count = 0;
    while(curr != NULL) {
        count++;
        if (curr != spaceship) {
            SDL_Rect r = curr->entity->rect;
            if (r.y + r.h < 0 || r.y - r.h > SCREEN_HEIGHT) {
                Spaceship *to_destroy = curr;
                curr = curr->next;
                last->next = curr;
                Spaceship_destroy(to_destroy);
            }
        }
        if (curr != NULL) {
            last = curr;
            curr = curr->next;
        }
    }

    if (count >= ENEMIES_COUNT) {
        return;
    }

    curr = last;
    SDL_Texture *texture = sdl_load_texture(renderer, "assets/spaceship.png");
    Uint32 enemies_to_spawn = MIN(ENEMIES_COUNT - count, MAX_SPAWN);
    for (int i = 0; i < enemies_to_spawn; i++) {
        SDL_Rect rect;
        rect.w = SPACESHIP_WIDTH / 1.5;
        rect.h = SPACESHIP_HEIGHT / 1.5;
        rect.x = rand() % (SCREEN_WIDTH - rect.w);
        rect.y = -rect.h;
        Entity *entity = Entity_new(180, 0, 1, rect, texture);
        curr->next = Spaceship_new(entity, ENEMY_HEALTH);
        curr = curr->next;
    }
}

bool Spaceship_in_firing_range(Spaceship *shooter, Spaceship *victim, SDL_Renderer *renderer) {
    SDL_Rect *shooter_r = &shooter->entity->rect, *victim_r = &victim->entity->rect;
    SDL_Rect bullet_rect;
    bullet_rect.x = shooter_r->x + (shooter_r->w / 2.) - (BULLET_WIDTH / 2.);
    bullet_rect.y = shooter_r->y + shooter_r->h;
    bullet_rect.w = BULLET_WIDTH;
    bullet_rect.h = SCREEN_HEIGHT;
    if (DEBUG) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderDrawRect(renderer, &bullet_rect);
    }
    return SDL_HasIntersection(victim_r, &bullet_rect);
}

void make_enemies_shoot(Spaceship *spaceship, BulletsManager *bullets_manager, SDL_Renderer *renderer) {
    Spaceship *curr = spaceship->next;
    while (curr != NULL) {
        curr->fire = false;
        if (rand() % 500 == 0) {
            curr->fire = true;
            Spaceship *curr2 = spaceship->next;
            while(curr2 != NULL) {
                if (curr2 != curr) {
                    if (Spaceship_in_firing_range(curr, curr2, renderer)) {
                        curr->fire = false;
                        break;
                    }
                }
                curr2 = curr2->next;
            }
        }
        Spaceship_fire(curr, bullets_manager, renderer, true);
        curr = curr->next;
    }
}

void move_spaceships(Spaceship *spaceship) {
    Spaceship *curr = spaceship;
    while(curr != NULL) {
        Entity_move(curr->entity);
        curr = curr->next;
    }
    SDL_Rect *player_r = &spaceship->entity->rect;
    if (player_r->x + (player_r->w / 2.) <= 0) {
        player_r->x = -(player_r->w / 2.);
    }
    if (player_r->x + (player_r->w / 2.) >= SCREEN_WIDTH) {
        player_r->x = SCREEN_WIDTH - (player_r->w / 2.);
    }
    /* if (player_r->y < 0) { player_r->y == 0; } */
}

/* void initialize_sdl(SDL_Window *window, SDL_Renderer *renderer) { */
void initialize_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        sdl_fail();
    }
    // nearest is better for pixel art (other is "linear"
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        sdl_fail();
    }
}

bool handle_input(Spaceship *player) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                return false;
            case SDL_KEYDOWN:
                if (event.key.repeat != 0) break;
                if (event.key.keysym.scancode == SDL_SCANCODE_Q) exit(0);
                if (event.key.keysym.scancode == SDL_SCANCODE_UP) player->entity->dy -= SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) player->entity->dy += SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) player->entity->dx -= SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) player->entity->dx += SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) player->fire = true;
                break;
            case SDL_KEYUP:
                if (event.key.keysym.scancode == SDL_SCANCODE_UP) player->entity->dy += SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) player->entity->dy -= SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) player->entity->dx += SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) player->entity->dx -= SPEED;
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) player->fire = false;
                break;
        }
    }
    return true;
}


void cap_fps(Uint32 max_fps) {
    static Uint32 ticks = 0;
    if (ticks == 0) { ticks = SDL_GetTicks(); }
    Uint32 new_ticks = SDL_GetTicks();
    Uint32 default_ = 1000. / max_fps;  // delay between frames assuming 0ms to calculate each frame
    Uint32 diff = new_ticks - ticks;  // time to calculate last frame
    if (default_ > diff) {
        // only wait when the wait time is > 0
        SDL_Delay(default_ - diff);
    }
    ticks = new_ticks;
}

typedef struct {
    char c;
    SDL_Rect rect; // rectangle on a texture with the character `c`
} FontChar;

typedef struct {
    FontChar *font_chars;
    char first_char;
    char last_char;
    char *texture_file_name;
} Font;

FontChar ken_pixel_font_chars[] = {
        {' ', {0, 0, 11, 54}},
        {'!', {11, 0, 20, 54}},
        {'"', {31, 0, 28, 54}},
        {'#', {59, 0, 37, 54}},
        {'$', {96, 0, 37, 54}},
        {'%', {133, 0, 37, 54}},
        {'&', {170, 0, 41, 54}},
        {'\'', {211, 0, 20, 54}},
        {'(', {231, 0, 24, 54}},
        {')', {255, 0, 24, 54}},
        {'*', {279, 0, 33, 54}},
        {'+', {311, 0, 37, 54}},
        {',', {348, 0, 20, 54}},
        {'-', {368, 0, 37, 54}},
        {'.', {405, 0, 20, 54}},
        {'/', {424, 0, 37, 54}},
        {'0', {461, 0, 37, 54}},
        {'1', {0, 54, 28, 54}},
        {'2', {28, 54, 37, 54}},
        {'3', {65, 54, 37, 54}},
        {'4', {102, 54, 37, 54}},
        {'5', {139, 54, 37, 54}},
        {'6', {176, 54, 37, 54}},
        {'7', {213, 54, 37, 54}},
        {'8', {249, 54, 37, 54}},
        {'9', {286, 54, 37, 54}},
        {':', {323, 54, 20, 54}},
        {';', {343, 54, 20, 54}},
        {'<', {363, 54, 33, 54}},
        {'=', {395, 54, 37, 54}},
        {'>', {432, 54, 33, 54}},
        {'?', {465, 54, 37, 54}},
        {'@', {0, 108, 37, 54}},
        {'A', {37, 108, 37, 54}},
        {'B', {74, 108, 37, 54}},
        {'C', {111, 108, 37, 54}},
        {'D', {147, 108, 37, 54}},
        {'E', {184, 108, 37, 54}},
        {'F', {221, 108, 37, 54}},
        {'G', {258, 108, 37, 54}},
        {'H', {295, 108, 37, 54}},
        {'I', {332, 108, 28, 54}},
        {'J', {360, 108, 28, 54}},
        {'K', {388, 108, 37, 54}},
        {'L', {425, 108, 33, 54}},
        {'M', {458, 108, 45, 54}},
        {'N', {0, 162, 37, 54}},
        {'O', {37, 162, 37, 54}},
        {'P', {74, 162, 37, 54}},
        {'Q', {111, 162, 37, 54}},
        {'R', {147, 162, 37, 54}},
        {'S', {184, 162, 37, 54}},
        {'T', {221, 162, 37, 54}},
        {'U', {258, 162, 37, 54}},
        {'V', {295, 162, 37, 54}},
        {'W', {332, 162, 45, 54}},
        {'X', {377, 162, 37, 54}},
        {'Y', {414, 162, 37, 54}},
        {'Z', {451, 162, 37, 54}},
        {'[', {488, 162, 24, 54}},
        {'\\', {0, 217, 37, 54}},
        {']', {37, 217, 24, 54}},
        {'^', {61, 217, 37, 54}},
        {'_', {98, 217, 37, 54}},
        {'`', {135, 217, 37, 54}},
        {'a', {171, 217, 37, 54}},
        {'b', {208, 217, 37, 54}},
        {'c', {245, 217, 37, 54}},
        {'d', {282, 217, 37, 54}},
        {'e', {319, 217, 37, 54}},
        {'f', {356, 217, 37, 54}},
        {'g', {393, 217, 37, 54}},
        {'h', {429, 217, 37, 54}},
        {'i', {466, 217, 28, 54}},
        {'j', {0, 271, 28, 54}},
        {'k', {28, 271, 37, 54}},
        {'l', {65, 271, 33, 54}},
        {'m', {98, 271, 45, 54}},
        {'n', {143, 271, 37, 54}},
        {'o', {180, 271, 37, 54}},
        {'p', {217, 271, 37, 54}},
        {'q', {254, 271, 37, 54}},
        {'r', {291, 271, 37, 54}},
        {'s', {327, 271, 37, 54}},
        {'t', {364, 271, 37, 54}},
        {'u', {401, 271, 37, 54}},
        {'v', {438, 271, 37, 54}},
        {'w', {0, 325, 45, 54}},
        {'x', {45, 325, 37, 54}},
        {'y', {82, 325, 37, 54}},
        {'z', {119, 325, 37, 54}},
        {'{', {156, 325, 28, 54}},
        {'|', {184, 325, 20, 54}},
        {'}', {204, 325, 28, 54}}
    };

Font ken_pixel_font = {ken_pixel_font_chars, ' ', '}', "assets/KenPixel.png" };

typedef struct {
    char *text;
    int x, y;
    float scale;
    Font *font;
} Text;

Uint32 calculate_width(Text *text) {
    Uint32 width = 0, i = 0;
    char c;
    while((c = text->text[i++]) != '\0') {
        width += text->scale * text->font->font_chars[c - text->font->first_char].rect.w;
    }
    return width;
}

void write_to_screen(SDL_Renderer *renderer, Text *text) {
    Font *font = text->font;
    SDL_Texture *texture = sdl_load_texture(renderer, font->texture_file_name);
    char c;
    int i = 0, w = text->x;
    while ((c = text->text[i++]) != '\0') {
        if (c < font->first_char || c > font->last_char) {
            printf("Warning: can't print char: %c (%d)\n", c, c);
        }
        SDL_Rect *src_rect = &font->font_chars[c - font->first_char].rect;
        SDL_Rect dst_rect;
        dst_rect.x = w;
        dst_rect.y = text->y;
        dst_rect.w = src_rect->w * text->scale;
        dst_rect.h = src_rect->h * text->scale;
        SDL_RenderCopy(renderer, texture, src_rect, &dst_rect);
        w += dst_rect.w;
    }
}

void show_fps(SDL_Renderer *renderer, SDL_Texture *font_texture, Uint32 fps) {
    char fps_str[20]; sprintf(fps_str, "fps: %d", fps);
    /* printf("%s\n", fps_str); */
    Text text = {fps_str, 0, 10, 1./2, &ken_pixel_font};
    Uint32 width = calculate_width(&text);
    text.x = SCREEN_WIDTH - width - 10;
    write_to_screen(renderer, &text);
}


int main(int argc, char *argv[]) {
    srand(12);
    initialize_sdl();

    // TODO: move window and renderer initialization to `initialize_sdl`
    // (for some reason it doesn't work properly when I move it...)
    SDL_Window *window = SDL_CreateWindow(
        "Spaceship",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0
    );
    if (!window) { sdl_fail(); }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { sdl_fail(); }

    SDL_Texture *font_texture = sdl_load_texture(renderer, "assets/KenPixel.png");

    Spaceship *player = Spaceship_new_player_spaceship(renderer);
    BulletsManager *bullets_manager = BulletsManager_new();

    Spaceship *curr = player;
    Uint32 spawn_delay = SPAWN_DELAY;
    Uint32 ticks = SDL_GetTicks();
    Uint32 fps = MAX_FPS;

    while(1) {
        if (player->health == 0) {
            printf("GAME OVER!!!\n");
            break;
        }


        if (!handle_input(player)) { break; }

        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderClear(renderer);


        move_spaceships(player);
        BulletsManager_move_bullets(bullets_manager);
        Spaceship_fire(player, bullets_manager, renderer, false);
        apply_bullet_hits(player, bullets_manager);
        make_enemies_shoot(player, bullets_manager, renderer);
        if (spawn_delay == 0) {
            spawn_enemies(player, renderer);
            spawn_delay = SPAWN_DELAY;
        }
        spawn_delay--;

        Spaceship_render(player, renderer);
        curr = player;
        while(curr != NULL) {
            Spaceship_render(curr, renderer);
            curr = curr->next;
        }
        BulletsManager_render_bullets(bullets_manager, renderer);
        Uint32 diff = SDL_GetTicks() - ticks;
        if (diff != 0) {
            fps = 1000. / diff;
            if (fps > MAX_FPS) {
                fps = MAX_FPS;
            }
        }
        show_fps(renderer, font_texture, fps);
        ticks = SDL_GetTicks();
        SDL_RenderPresent(renderer);
        cap_fps(MAX_FPS);
    }

    Spaceship_destroy(player);
    BulletsManager_destroy(bullets_manager);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}