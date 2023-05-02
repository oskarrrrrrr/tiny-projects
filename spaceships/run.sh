gcc main.c \
    -o spaceships \
    -Wall -Wextra -Wunreachable-code \
    `pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer` \
    -DSDL_DISABLE_IMMINTRIN_H \
    && ./spaceships
