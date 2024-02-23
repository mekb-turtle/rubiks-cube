#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include "err.h"
#include <math.h>
#include "render.h"
#include "./config.h"

static const float colors[6 * 3] = {
        COLOR_0,
        COLOR_1,
        COLOR_2,
        COLOR_3,
        COLOR_4,
        COLOR_5,
};

static const float whole_size = 0.2f;
static const float sticker_size = 0.475f;
static const float cube_size = 1.5f;
static const float inner_size = 0.003f;
static const float outer_size = 0;
static const float gap_size = 2.0f;

static float pitch = 20;
static float yaw = 20;

void rotate_camera(float x, float y) {
	yaw -= x;
	pitch += y;
	if (yaw > 360) yaw -= 360;
	if (yaw < -360) yaw += 360;
	if (pitch > 90) pitch = 90;
	if (pitch < -90) pitch = -90;
}

static size_t rotations_i = 0;
static struct sticker_rotations sticker_rotations[MAX_ANIMATIONS];

void initialize_animations() {
	memset(sticker_rotations, 0, sizeof(sticker_rotations));
}

void send_animation(struct sticker_rotations animation) {
	sticker_rotations[rotations_i] = animation;

	// acts as a ring buffer
	++rotations_i;
	rotations_i %= MAX_ANIMATIONS;
}

void render(struct cube *cube) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
    glEnable(GL_MULTISAMPLE);
	glLoadIdentity();

	glRotatef(pitch, -1.0f, 0.0f, 0.0f);

	glRotatef(yaw, 0.0f, -1.0f, 0.0f);

	glScalef(whole_size, whole_size, whole_size);

	for (intpos face_i = 0; face_i < 6; ++face_i) {
		glPushMatrix();

		switch (face_i) {
			case 0:
				glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
				break;
			case 1:
				break;
			case 2:
				glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
				break;
			case 3:
				glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
				break;
			case 4:
				glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
				break;
			case 5:
				glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
				break;
		}

		//TODO: animate making a move
		glBegin(GL_QUADS);
		for (intpos sticker_i = 0; sticker_i < 9; ++sticker_i) {
			struct face *face = &cube->faces[face_i];
			face_color *sticker = &face->stickers[sticker_i];
			const float *c = &colors[*sticker * 3];
			glColor3f(c[0], c[1], c[2]);

			float s_x = (float) (sticker_i % 3) - 1;
			float s_y = (float) (sticker_i / 3) - 1;

			// draw on cube
			glVertex3f(s_x - sticker_size, s_y - sticker_size, cube_size + outer_size);
			glVertex3f(s_x - sticker_size, s_y + sticker_size, cube_size + outer_size);
			glVertex3f(s_x + sticker_size, s_y + sticker_size, cube_size + outer_size);
			glVertex3f(s_x + sticker_size, s_y - sticker_size, cube_size + outer_size);

			// draw out of cube (for seeing back faces)
			glVertex3f(s_x - sticker_size, s_y - sticker_size, cube_size + gap_size);
			glVertex3f(s_x + sticker_size, s_y - sticker_size, cube_size + gap_size);
			glVertex3f(s_x + sticker_size, s_y + sticker_size, cube_size + gap_size);
			glVertex3f(s_x - sticker_size, s_y + sticker_size, cube_size + gap_size);
		}

		glColor3f(0, 0, 0);
		glVertex3f(-cube_size, -cube_size, cube_size - inner_size);
		glVertex3f(-cube_size, +cube_size, cube_size - inner_size);
		glVertex3f(+cube_size, +cube_size, cube_size - inner_size);
		glVertex3f(+cube_size, -cube_size, cube_size - inner_size);
		glEnd();

		glPopMatrix();
	}
}
