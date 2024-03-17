#include <stdio.h>
#include "err.h"
#include "render.h"
#include "util.h"
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#define RENDER
#include "moves.h"
#include "config.h"

static float yaw, pitch;

void reset_camera() {
	yaw = -20, pitch = 20;
}

void rotate_camera(float x, float y) {
	yaw += x;
	pitch += y;
	if (yaw > 360) yaw -= 360;
	if (yaw < -360) yaw += 360;
	if (pitch > 90) pitch = 90;
	if (pitch < -90) pitch = -90;
}

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

static GLuint vbo_vertex_positions = 0, vbo_vertex_colors = 0, vbo_vertex_stickers = 0, vbo_vertex_texcoords = 0, vao = 0, shader_program = 0;

static const size_t vertices_per_rectangle = 4;
static const size_t rectangles_per_sticker = 5;
static const size_t vertices_total_count = 6 * 9 * rectangles_per_sticker * vertices_per_rectangle;
static const size_t vertices_size = vertices_total_count * 3 * sizeof(float);

static void add_uint8(uint8_t *values, size_t *len, uint8_t value) {
	values[*len] = value;
	(*len)++;
}

static void add_float(float *values, size_t *len, float value) {
	values[*len] = value;
	(*len)++;
}

static void add_rect_uint8(uint8_t *values, size_t *len, uint8_t value) {
	for (intpos i = 0; i < vertices_per_rectangle; ++i)
		add_uint8(values, len, value);
}

static void add_vec3(float *values, size_t *len, struct vec3 vec3) {
	for (intpos j = 0; j < 3; ++j)
		add_float(values, len, vec3.points[j]);
}

static void add_rect(float *values, size_t *len, struct rect rect) {
	add_vec3(values, len, rect.vec[0]);
	add_vec3(values, len, rect.vec[1]);
	add_vec3(values, len, rect.vec[2]);
	add_vec3(values, len, rect.vec[3]);
}

static const float rect_texcoord[8] = { // texture coordinates for a rect
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};

static void add_texcoord(float *values, size_t *len) {
	for (intpos i = 0; i < sizeof(rect_texcoord) / sizeof(rect_texcoord[0]); ++i) {
		add_float(values, len, rect_texcoord[i]);
	}
}

static float *vertex_colors = NULL;

void unload() {
	if (vao) {
		glBindVertexArray(vao);
		if (vbo_vertex_positions) glDeleteBuffers(1, &vbo_vertex_positions);
		if (vbo_vertex_colors) glDeleteBuffers(1, &vbo_vertex_colors);
		if (vbo_vertex_stickers) glDeleteBuffers(1, &vbo_vertex_stickers);
		if (vbo_vertex_texcoords) glDeleteBuffers(1, &vbo_vertex_texcoords);
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
	}
	if (shader_program) glDeleteProgram(shader_program);
	if (vertex_colors) free(vertex_colors);
}

extern const struct move_map moves_map[][4];
extern const intpos faces_map[];

static GLuint animation[4] = {0, 0, 0, 0}; // current animation

static bool update_animation() {
	glUseProgram(shader_program);
	GLint animation_uniform = glGetUniformLocation(shader_program, "animation");
	if (animation_uniform >= 0) glUniform4uiv(animation_uniform, 1, animation);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		warnx("OpenGL error: %i", error);
		return false;
	}
	return true;
}

static GLuint *get_animation(struct sticker_rotations ani) {
	// compact all the data of the struct into a uvec4
	return (GLuint[4]){
	        ani.axis - AXIS_X + (ani.dir * 3),
	        ani.stickers,
	        ani.stickers >> 32,
	        ani.start_time};
}

bool send_animation(struct sticker_rotations ani) {
	GLuint *anim = get_animation(ani);

	// copy the uvec4
	for (int i = 0; i < 4; i++) {
		animation[i] = anim[i];
	}

	return update_animation();
}

bool initialize_render() {
	reset_camera();

	GLuint vertex_shader = 0, fragment_shader = 0;

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
	size_t vertex_position_i = 0;
	float *vertex_positions = malloc(vertices_size);
	if (!vertex_positions) {
		warn("Failed to allocate vertex position buffer");
		goto error;
	}

	// stores sticker indices of each vertex, used for rotations
	size_t vertex_sticker_i = 0;
	const size_t vertices_sticker_size = vertices_total_count * sizeof(uint8_t);
	uint8_t *vertex_stickers = malloc(vertices_sticker_size);
	if (!vertex_stickers) {
		warn("Failed to allocate vertex sticker indices buffer");
		goto error;
	}

	// stores texture coordinates for each vertex
	size_t vertex_texcoord_i = 0;
	const size_t vertices_texcoord_size = vertices_total_count * 2 * sizeof(float);
	float *vertex_texcoords = malloc(vertices_texcoord_size);
	if (!vertex_texcoords) {
		warn("Failed to allocate vertex texcoord buffer");
		goto error;
	}

	// initialize vertices
	// TODO: put rectangles inside of the cube so the
	// user cannot see through the cube when rotating
	for (intpos face_i = 0; face_i < 6; ++face_i) {
		for (intpos sticker_i = 0; sticker_i < 9; ++sticker_i) {
			float s_x = ((float) (sticker_i % 3) - 1) * sticker_distance;
			float s_y = ((float) (sticker_i / 3) - 1) * sticker_distance;

			// change rectangles_per_sticker if you want to add or remove rectangles here
			// you can adjust the following values in config.h

			// square on the cube
			add_rect(vertex_positions, &vertex_position_i, transform_rect(rect(vec3(s_x - sticker_size, s_y - sticker_size, cube_size + outwards_offset), vec3(s_x - sticker_size, s_y + sticker_size, cube_size + outwards_offset), vec3(s_x + sticker_size, s_y + sticker_size, cube_size + outwards_offset), vec3(s_x + sticker_size, s_y - sticker_size, cube_size + outwards_offset)), face_i, cube_scale));

			// square for the back faces
			add_rect(vertex_positions, &vertex_position_i, transform_rect(rect(vec3(s_x - sticker_size, s_y - sticker_size, cube_size + back_face_distance - outwards_offset), vec3(s_x + sticker_size, s_y - sticker_size, cube_size + back_face_distance - outwards_offset), vec3(s_x + sticker_size, s_y + sticker_size, cube_size + back_face_distance - outwards_offset), vec3(s_x - sticker_size, s_y + sticker_size, cube_size + back_face_distance - outwards_offset)), face_i, cube_scale));

			// black border to prevent seeing inside cube
			add_rect(vertex_positions, &vertex_position_i, transform_rect(rect(vec3(s_x - sticker_inner_size, s_y - sticker_inner_size, cube_size + inwards_offset), vec3(s_x - sticker_inner_size, s_y + sticker_inner_size, cube_size + inwards_offset), vec3(s_x + sticker_inner_size, s_y + sticker_inner_size, cube_size + inwards_offset), vec3(s_x + sticker_inner_size, s_y - sticker_inner_size, cube_size + inwards_offset)), face_i, cube_scale));

			// same as above but flipped to prevent seeing inside when rotating
			add_rect(vertex_positions, &vertex_position_i, transform_rect(rect(vec3(s_x - sticker_inner_size, s_y - sticker_inner_size, cube_size + inwards_offset), vec3(s_x + sticker_inner_size, s_y - sticker_inner_size, cube_size + inwards_offset), vec3(s_x + sticker_inner_size, s_y + sticker_inner_size, cube_size + inwards_offset), vec3(s_x - sticker_inner_size, s_y + sticker_inner_size, cube_size + inwards_offset)), face_i, cube_scale));

			// black border but for the back faces
			add_rect(vertex_positions, &vertex_position_i, transform_rect(rect(vec3(s_x - sticker_inner_size, s_y - sticker_inner_size, cube_size + back_face_distance - inwards_offset), vec3(s_x + sticker_inner_size, s_y - sticker_inner_size, cube_size + back_face_distance - inwards_offset), vec3(s_x + sticker_inner_size, s_y + sticker_inner_size, cube_size + back_face_distance - inwards_offset), vec3(s_x - sticker_inner_size, s_y + sticker_inner_size, cube_size + back_face_distance - inwards_offset)), face_i, cube_scale));

			uint8_t sticker_index = get_sticker_index(face_i, sticker_i);
			for (intpos times = 0; times < rectangles_per_sticker; ++times) {
				// send sticker index to vertex buffer
				add_rect_uint8(vertex_stickers, &vertex_sticker_i, sticker_index);
				add_texcoord(vertex_texcoords, &vertex_texcoord_i);
			}
		}
	}

	// VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Vertex position buffer
	glGenBuffers(1, &vbo_vertex_positions);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_positions);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertex_positions, GL_STATIC_DRAW);
	free(vertex_positions);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// stores color of each vertex
	// luckily XYZ is same size as RGB so we can use the same size
	vertex_colors = malloc(vertices_size);
	if (!vertex_colors) {
		warn("Failed to allocate vertex color buffer");
		goto error;
	}
	memset(vertex_colors, 0, vertices_size);

	// Vertex color buffer
	glGenBuffers(1, &vbo_vertex_colors);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_colors);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertex_colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(1);

	// Vertex sticker index buffer
	glGenBuffers(1, &vbo_vertex_stickers);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_stickers);
	glBufferData(GL_ARRAY_BUFFER, vertices_sticker_size, vertex_stickers, GL_STATIC_DRAW);
	free(vertex_stickers);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(uint8_t), 0);
	glEnableVertexAttribArray(2);

	// Vertex texture coordinate buffer
	glGenBuffers(1, &vbo_vertex_texcoords);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_texcoords);
	glBufferData(GL_ARRAY_BUFFER, vertices_texcoord_size, vertex_texcoords, GL_STATIC_DRAW);
	free(vertex_texcoords);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	if (!update_animation()) goto error;

	update_render_turn_time();

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		warnx("OpenGL error: %i", error);
		goto error;
	}
	return true;
error:
	if (shader_program) glDeleteProgram(shader_program);
	if (vertex_shader) glDeleteShader(vertex_shader);
	if (fragment_shader) glDeleteShader(fragment_shader);
	unload();

	return false;
}

bool update_cube(struct cube *cube) {
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
			add_rect(vertex_colors, &vertex_i, inner_color_rect);
		}
	}

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_colors);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertex_colors, GL_STATIC_DRAW);
	glBindVertexArray(0);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		warnx("OpenGL error: %i", error);
		return false;
	}
	return true;
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

	GLint look_uniform = glGetUniformLocation(shader_program, "look");
	if (look_uniform >= 0) glUniform2f(look_uniform, yaw, pitch);

	GLint time_uniform = glGetUniformLocation(shader_program, "time");
	if (time_uniform >= 0) glUniform1ui(time_uniform, SDL_GetTicks());

	glBindVertexArray(vao);
	glDrawArrays(GL_QUADS, 0, vertices_total_count);
	glBindVertexArray(0);
}

void update_render_turn_time() {
	if (!shader_program) return;
	GLint turn_time_uniform = glGetUniformLocation(shader_program, "turnTime");
	if (turn_time_uniform >= 0) glUniform1ui(turn_time_uniform, current_turn_time);
}
