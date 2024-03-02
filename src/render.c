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

// vertex generation for the cube

struct tri {
	struct vec3 {
		union {
			struct {
				float x, y, z;
			};
			float points[3];
		};
	} vec[3];
};

struct rect {
	struct vec3 vec[4];
};

static struct vec3 vec3(float x, float y, float z) {
	return (struct vec3){{{x, y, z}}};
}

static struct tri tri(struct vec3 a, struct vec3 b, struct vec3 c) {
	return (struct tri){
	        {a, b, c}
    };
}

static struct rect rect(struct vec3 a, struct vec3 b, struct vec3 c, struct vec3 d) {
	return (struct rect){
	        {a, b, c, d}
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

static GLuint vbo_vertices = 0, vao_vertices = 0, shader_program = 0;

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

	const size_t vertices_size = 120 * 18 * sizeof(float);
	size_t vertex_i = 0;
	float *vertices = malloc(vertices_size); // 20 rectangles for 6 sides, every rectangle is 2 polygons which is 3 vertices each, which have 3 values for xyz
	if (!vertices) {
		warn("Failed to allocate vertices");
		goto error;
	}

	for (intpos face_i = 0; face_i < 6; ++face_i) {
		for (intpos sticker_i = 0; sticker_i < 9; ++sticker_i) {
			float s_x = (float) (sticker_i % 3) - 1;
			float s_y = (float) (sticker_i / 3) - 1;

			// square on the cube
			add_rect(vertices, &vertex_i, transform_rect(rect(vec3(s_x - sticker_size, s_y - sticker_size, cube_size + outer_size), vec3(s_x - sticker_size, s_y + sticker_size, cube_size + outer_size), vec3(s_x + sticker_size, s_y + sticker_size, cube_size + outer_size), vec3(s_x + sticker_size, s_y - sticker_size, cube_size + outer_size)), face_i, cube_scale));

			// square out of the cube (for seeing back faces)
			add_rect(vertices, &vertex_i, transform_rect(rect(vec3(s_x - sticker_size, s_y - sticker_size, cube_size + gap_size - outer_size), vec3(s_x + sticker_size, s_y - sticker_size, cube_size + gap_size - outer_size), vec3(s_x + sticker_size, s_y + sticker_size, cube_size + gap_size - outer_size), vec3(s_x - sticker_size, s_y + sticker_size, cube_size + gap_size - outer_size)), face_i, cube_scale));
		}

		// black border to prevent seeing inside cube
		add_rect(vertices, &vertex_i, transform_rect(rect(vec3(-cube_size, -cube_size, cube_size - inner_size), vec3(-cube_size, +cube_size, cube_size - inner_size), vec3(+cube_size, +cube_size, cube_size - inner_size), vec3(+cube_size, -cube_size, cube_size - inner_size)), face_i, cube_scale));

		// same but outside of cube
		add_rect(vertices, &vertex_i, transform_rect(rect(vec3(-cube_size, -cube_size, cube_size + gap_size + inner_size), vec3(-cube_size, +cube_size, cube_size + gap_size + inner_size), vec3(+cube_size, +cube_size, cube_size + gap_size + inner_size), vec3(+cube_size, -cube_size, cube_size + gap_size + inner_size)), face_i, cube_scale));
	}

	// Set up vertex data and buffers
	glGenVertexArrays(1, &vao_vertices);
	glGenBuffers(1, &vbo_vertices);
	glBindVertexArray(vao_vertices);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);

	// Set up vertex attribute pointers for positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	return true;
error:
	if (shader_program) glDeleteProgram(shader_program);
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
	glClearColor(0, 0, 0, 1.0);
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
