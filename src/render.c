#include <stdio.h>
#include "err.h"
#include "render.h"
#include "./config.h"
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

/*
static const float colors[6 * 3] = {
        COLOR_0,
        COLOR_1,
        COLOR_2,
        COLOR_3,
        COLOR_4,
        COLOR_5,
};
*/

static const float cube_scale = 0.2f;
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

struct vec3 {
	float x, y, z;
};

struct rect {
	union {
		struct {
			struct vec3 a, b, c, d;
		};
		struct vec3 vec[4];
	};
};

static struct rect rect(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float dx, float dy, float dz) {
	return (struct rect){
	        (struct vec3){ax, ay, az},
	        (struct vec3){bx, by, bz},
	        (struct vec3){cx, cy, cz},
	        (struct vec3){dx, dy, dz}
    };
}

static struct vec3 transform_vec3(struct vec3 vec, intpos face, float scale) {
	struct vec3 ret = vec;
	switch (face) {
		case 0:
			ret.y = -vec.z;
			ret.z = vec.y;
			break;
		case 1:
			break;
		case 2:
			ret.x = vec.z;
			ret.z = vec.x;
			break;
		case 3:
			ret.x = -vec.x;
			ret.z = -vec.z;
			break;
		case 4:
			ret.x = -vec.z;
			ret.z = -vec.x;
			break;
		case 5:
			ret.y = vec.z;
			ret.z = -vec.y;
			break;
	}
	ret.x *= scale;
	ret.y *= scale;
	ret.z *= scale;
	return ret;
}

static struct rect transform_rect(struct rect rect, intpos face, float scale) {
	for (intpos i = 0; i < 4; ++i) {
		rect.vec[i] = transform_vec3(rect.vec[i], face, scale);
	}
	return rect;
}

extern const char binary_shader_fsh[];
extern const char binary_shader_vsh[];
extern int binary_shader_fsh_len;
extern int binary_shader_vsh_len;

static GLuint vbo_vertices, vao_vertices, shader_program;

bool initialize_render() {
	GLuint vertex_shader = 0, fragment_shader = 0;

	// Clear rotation animation
	memset(sticker_rotations, 0, sizeof(sticker_rotations));

	int success;
	char infoLog[512];

	const char *const shader_fsh = binary_shader_fsh;
	const char *const shader_vsh = binary_shader_vsh;

	// Compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &shader_vsh, &binary_shader_vsh_len);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
		fprintf(stderr, "Error compiling vertex shader\n%s\n", infoLog);
		goto error;
	}

	// Compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &shader_fsh, &binary_shader_fsh_len);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
		fprintf(stderr, "Error compiling fragment shader\n%s\n", infoLog);
		goto error;
	}

	// Create shader program
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader_program, 512, NULL, infoLog);
		fprintf(stderr, "Error linking program\n%s\n", infoLog);
		goto error;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	// Set up vertex data and buffers
	struct rect rects[20 * 6] = {};

	for (intpos face_i = 0, rect_i = 0; face_i < 6; ++face_i) {
		for (intpos sticker_i = 0; sticker_i < 9; ++sticker_i) {
			float s_x = (float) (sticker_i % 3) - 1;
			float s_y = (float) (sticker_i / 3) - 1;

			// square on the cube
			rects[rect_i++] = transform_rect(rect(
			                                         s_x - sticker_size, s_y - sticker_size, cube_size + outer_size,
			                                         s_x - sticker_size, s_y + sticker_size, cube_size + outer_size,
			                                         s_x + sticker_size, s_y + sticker_size, cube_size + outer_size,
			                                         s_x + sticker_size, s_y - sticker_size, cube_size + outer_size),
			                                 face_i, cube_scale);

			// square out of the cube (for seeing back faces)
			rects[rect_i++] = transform_rect(rect(
			                                         s_x - sticker_size, s_y - sticker_size, cube_size + gap_size - outer_size,
			                                         s_x + sticker_size, s_y - sticker_size, cube_size + gap_size - outer_size,
			                                         s_x + sticker_size, s_y + sticker_size, cube_size + gap_size - outer_size,
			                                         s_x - sticker_size, s_y + sticker_size, cube_size + gap_size - outer_size),
			                                 face_i, cube_scale);
		}

		// black border to prevent seeing inside cube
		rects[rect_i++] = transform_rect(rect(
		                                         -cube_size, -cube_size, cube_size - inner_size,
		                                         -cube_size, +cube_size, cube_size - inner_size,
		                                         +cube_size, +cube_size, cube_size - inner_size,
		                                         +cube_size, -cube_size, cube_size - inner_size),
		                                 face_i, cube_scale);

		// same but outside of cube
		rects[rect_i++] = transform_rect(rect(
		                                         -cube_size, -cube_size, cube_size + gap_size + inner_size,
		                                         -cube_size, +cube_size, cube_size + gap_size + inner_size,
		                                         +cube_size, +cube_size, cube_size + gap_size + inner_size,
		                                         +cube_size, -cube_size, cube_size + gap_size + inner_size),
		                                 face_i, cube_scale);
	}

	for (int i = 0; i < (20 * 6); ++i) {
		struct rect rect = rects[i];
		printf("Rect %i:", i);
		for (int j = 0; j < 4; ++j) {
			struct vec3 vec = rect.vec[j];
			printf(" (%f %f %f)", vec.x, vec.y, vec.z);
		}
		printf("\n");
	}

	glGenVertexArrays(1, &vao_vertices);
	glGenBuffers(1, &vbo_vertices);
	glBindVertexArray(vao_vertices);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rects), rects, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	return true;
error:
	if (vertex_shader) glDeleteShader(vertex_shader);
	if (fragment_shader) glDeleteShader(fragment_shader);
	return false;
}

void send_animation(struct sticker_rotations animation) {
	sticker_rotations[rotations_i] = animation;

	// acts as a ring buffer
	++rotations_i;
	rotations_i %= MAX_ANIMATIONS;
}

void update_cube(struct cube *cube) {
}

void render() {
	// Clear
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_MULTISAMPLE);

	// Draw program
	glUseProgram(shader_program);
	GLint look = glGetUniformLocation(shader_program, "look");
	if (look >= 0) glUniform2f(look, yaw, pitch);
	glBindVertexArray(vao_vertices);
	glDrawArrays(GL_QUADS, 0, 4);
	return;
	/*

	glLoadIdentity();

	glRotatef(pitch, -1.0f, 0.0f, 0.0f);

	glRotatef(yaw, 0.0f, -1.0f, 0.0f);

	glScalef(whole_size, whole_size, whole_size);

	for (intpos face_i = 0; face_i < 6; ++face_i) {
		glPushMatrix();


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
	}*/
}
