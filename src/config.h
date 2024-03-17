#ifndef CONFIG_H
#define CONFIG_H
#define WINDOW_TITLE "Rubik's Cube"
#include <stdint.h>

typedef uint32_t int_time; // what type to use for time (should be the same as SDL and GL, so GLuint and Uint32)
static const int_time turn_time = 400, turn_time_shuffle = 150;
static const uint32_t max_moves = 4;

#ifdef RENDER
static const float
        cube_scale = 0.2f, // scale of entire cube, all other values are scaled by this

        sticker_size = 0.475f,     // size of sticker (the colored rectangle)
        sticker_inner_size = 0.5f, // size of the inner rectangles around stickers
        cube_size = 1.5f,          // distance from origin to a face, should be sticker_distance*1.5
        sticker_distance = 1.0f,   // distance from each sticker
        inwards_offset = -0.003f,  // inwards offset of the inner rectangles around sticker
        outwards_offset = 0.003f,  // outwards offset of sticker, cannot be equal to inwards_offset otherwise z-fighting, cannot be less than
        back_face_distance = 2.0f; // distance away to back faces, visible when the normal faces are obscured

static const struct vec3
        background_color = {{{0.0, 0.0, 0.0}}},
        colors[] = {
                {{{0.0, 0.0, 0.0}}}, // inner rectangle: a slightly bigger rectangle that goes behind stickers to prevent seeing through the cube
                {{{1.0, 1.0, 1.0}}}, // top
                {{{1.0, 0.0, 0.0}}}, // front
                {{{0.0, 0.0, 1.0}}}, // right
                {{{1.0, 0.5, 0.0}}}, // back
                {{{0.0, 1.0, 0.0}}}, // left
                {{{1.0, 1.0, 0.0}}}  // bottom
};
#endif
#endif //CONFIG_H
