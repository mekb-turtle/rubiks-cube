#include <stdio.h>
#include "err.h"
#include "render.h"
#include "util.h"
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#define RENDER
#include "./config.h"

static float yaw = -20;
static float pitch = 20;

void rotate_camera(float x, float y) {
	yaw += x;
	pitch += y;
	if (yaw > 360) yaw -= 360;
	if (yaw < -360) yaw += 360;
	if (pitch > 90) pitch = 90;
	if (pitch < -90) pitch = -90;
}

// maximum amount of animations at a time
// make sure this is the same as in the vertex shader
#define MAX_ANIMATIONS 6

static size_t rotations_i = 0;
static struct sticker_rotations sticker_rotations[MAX_ANIMATIONS];

// vertex generation for the cube


static struct vec3 transform_vec3(struct vec3 vec, intpos face, float scale) {
	struct vec3 ret = vec;

	// transform a vec3 so it's on a certain face
	switch (face) {
		case 0:
			ret.y = -vec.z;
			ret.z = vec.y;
			break;
		case 1:
			break;
		case 2:
			ret.x = vec.z;
			ret.z = -vec.x;
			break;
		case 3:
			ret.x = -vec.x;
			ret.z = -vec.z;
			break;
		case 4:
			ret.x = -vec.z;
			ret.z = vec.x;
			break;
		case 5:
			ret.y = vec.z;
			ret.z = -vec.y;
			break;
	}

	// scale the vector
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

static GLuint vbo_vertex_positions = 0, vbo_vertex_colors = 0, vao = 0, shader_program = 0;

static void add_vec3(float *values, size_t *len, struct vec3 vec3) {
	for (intpos j = 0; j < 3; ++j) {
		float f = vec3.points[j];
		values[*len] = f;
		(*len)++;
	}
}

static void add_tri(float *values, size_t *len, struct tri tri) {
	add_vec3(values, len, tri.vec[0]);
	add_vec3(values, len, tri.vec[1]);
	add_vec3(values, len, tri.vec[2]);
}

static void add_rect(float *values, size_t *len, struct rect rect) {
	add_tri(values, len, tri(rect.vec[0], rect.vec[1], rect.vec[3]));
	add_tri(values, len, tri(rect.vec[1], rect.vec[2], rect.vec[3]));
}

// 9 stickers with 4 rectangles each, 6 sides, every rectangle is 2 polygons which is 3 vertices each, which have 3 values for xyz
static const size_t vertices_count = 9 * 4 * 6 * 2 * 3;
static const size_t vertices_size = vertices_count * 3 * sizeof(float);

static float *vertex_colors = NULL;

void unload() {
	if (shader_program) glDeleteProgram(shader_program);
	if (vertex_colors) free(vertex_colors);
}

static void send_color_vbo();

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

	// stores position of each vertex
	size_t vertex_i = 0;
	float *vertex_positions = malloc(vertices_size);
	if (!vertex_positions) {
		warn("Failed to allocate vertex positions");
		goto error;
	}

	// initialize vertices
	for (intpos face_i = 0; face_i < 6; ++face_i) {
		for (intpos sticker_i = 0; sticker_i < 9; ++sticker_i) {
			float s_x = ((float) (sticker_i % 3) - 1) * sticker_distance;
			float s_y = ((float) (sticker_i / 3) - 1) * sticker_distance;

			// square on the cube
			add_rect(vertex_positions, &vertex_i, transform_rect(rect(vec3(s_x - sticker_size, s_y - sticker_size, cube_size + outwards_offset), vec3(s_x - sticker_size, s_y + sticker_size, cube_size + outwards_offset), vec3(s_x + sticker_size, s_y + sticker_size, cube_size + outwards_offset), vec3(s_x + sticker_size, s_y - sticker_size, cube_size + outwards_offset)), face_i, cube_scale));

			// square out of the cube (for seeing back faces)
			add_rect(vertex_positions, &vertex_i, transform_rect(rect(vec3(s_x - sticker_size, s_y - sticker_size, cube_size + back_face_distance - outwards_offset), vec3(s_x + sticker_size, s_y - sticker_size, cube_size + back_face_distance - outwards_offset), vec3(s_x + sticker_size, s_y + sticker_size, cube_size + back_face_distance - outwards_offset), vec3(s_x - sticker_size, s_y + sticker_size, cube_size + back_face_distance - outwards_offset)), face_i, cube_scale));

			// black border to prevent seeing inside cube
			add_rect(vertex_positions, &vertex_i, transform_rect(rect(vec3(s_x - sticker_inner_size, s_y - sticker_inner_size, cube_size + inwards_offset), vec3(s_x - sticker_inner_size, s_y + sticker_inner_size, cube_size + inwards_offset), vec3(s_x + sticker_inner_size, s_y + sticker_inner_size, cube_size + inwards_offset), vec3(s_x + sticker_inner_size, s_y - sticker_inner_size, cube_size + inwards_offset)), face_i, cube_scale));

			// same but outside of cube
			add_rect(vertex_positions, &vertex_i, transform_rect(rect(vec3(s_x - sticker_inner_size, s_y - sticker_inner_size, cube_size + back_face_distance - inwards_offset), vec3(s_x + sticker_inner_size, s_y - sticker_inner_size, cube_size + back_face_distance - inwards_offset), vec3(s_x + sticker_inner_size, s_y + sticker_inner_size, cube_size + back_face_distance - inwards_offset), vec3(s_x - sticker_inner_size, s_y + sticker_inner_size, cube_size + back_face_distance - inwards_offset)), face_i, cube_scale));
		}
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Set up vertex data and buffers
	glGenBuffers(1, &vbo_vertex_positions);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_positions);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertex_positions, GL_STATIC_DRAW);
	free(vertex_positions);

	// Set up vertex attribute pointers for positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(0);

	// stores color of each vertex
	// luckily XYZ is same size as RGB so we can use the same size
	vertex_colors = malloc(vertices_size);
	if (!vertex_colors) {
		warn("Failed to allocate vertex colors");
		goto error;
	}

	memset(vertex_colors, 0, vertices_size);

	glGenBuffers(1, &vbo_vertex_colors);
	send_color_vbo();

	glBindVertexArray(vao);
	// Set up color attribute pointers for positions
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return true;
error:
	if (shader_program) glDeleteProgram(shader_program);
	if (vertex_shader) glDeleteShader(vertex_shader);
	if (fragment_shader) glDeleteShader(fragment_shader);
	return false;
}

static void send_color_vbo() {
	// Set up color data and buffers
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_colors);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertex_colors, GL_STATIC_DRAW);
	glBindVertexArray(0);
}

void send_animation(struct sticker_rotations animation) {
	sticker_rotations[rotations_i] = animation;

	// acts as a ring buffer
	++rotations_i;
	rotations_i %= MAX_ANIMATIONS;
}

void update_cube(struct cube *cube) {
	// inner color (usually black)
	struct vec3 inner_color = colors[0];
	struct rect inner_color_rect = rect(inner_color, inner_color, inner_color, inner_color);

	size_t vertex_i = 0;
	for (intpos face_i = 0; face_i < 6; ++face_i) {
		for (intpos sticker_i = 0; sticker_i < 9; ++sticker_i) {
			// get color of sticker
			face_color face_color = cube->faces[face_i].stickers[sticker_i];
			struct vec3 color = colors[face_color + 1];
			struct rect color_rect = rect(color, color, color, color);
			add_rect(vertex_colors, &vertex_i, color_rect);
			add_rect(vertex_colors, &vertex_i, color_rect);
			add_rect(vertex_colors, &vertex_i, inner_color_rect);
			add_rect(vertex_colors, &vertex_i, inner_color_rect);
		}
	}

	send_color_vbo();
}

void render() {
	// Clear
	glClearColor(background_color.x, background_color.y, background_color.z, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	glEnable(GL_MULTISAMPLE); // Anti-aliasing

	// Draw program

	glUseProgram(shader_program);

	GLint look = glGetUniformLocation(shader_program, "look");
	if (look >= 0) glUniform2f(look, yaw, pitch);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertices_count);
	glBindVertexArray(0);
}
