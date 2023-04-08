clang main.c \
    -o spaceships \
    -g \
    `pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer` \
    -DSDL_DISABLE_IMMINTRIN_H \
    && ./spaceships
