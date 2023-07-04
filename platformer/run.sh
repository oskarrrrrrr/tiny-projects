gcc main.c SDL_utils.c stage.c \
    -o platformer \
    -g \
    -Wall -Wextra -Wunreachable-code \
    `pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf` \
    -DSDL_DISABLE_IMMINTRIN_H \
    && ./platformer
