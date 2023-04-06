clang main.c \
    -g \
    -o spaceships \
    `pkg-config --cflags --libs sdl2 SDL2_image` \
    -DSDL_DISABLE_IMMINTRIN_H \
    && ./spaceships
