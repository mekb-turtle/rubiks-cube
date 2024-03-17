#ifndef RENDER_H
#define RENDER_H
#include "rubik.h"
void reset_camera();
void rotate_camera(float x, float y);
void unload();
bool initialize_render();
bool send_animation(struct sticker_rotations animation);
bool update_cube(struct cube *cube);
void render();
void update_render_turn_time();
#endif
