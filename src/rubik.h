#ifndef RUBIK_H
#define RUBIK_H
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef uint8_t intpos;
typedef uint8_t face_color;

struct cube {
	union {
		struct face {
			union {
				face_color stickers[9];
				struct {
					face_color top_left;
					face_color top_center;
					face_color top_right;
					face_color middle_left;
					face_color middle_center;
					face_color middle_right;
					face_color bottom_left;
					face_color bottom_center;
					face_color bottom_right;
				};
			};
		} faces[6];
		face_color stickers[9 * 6];
	};
};

enum stickers {
	top_left,
	top_center,
	top_right,
	middle_left,
	middle_center,
	middle_right,
	bottom_left,
	bottom_center,
	bottom_right
};

struct move {
	enum move_face {
		NO_FACE = 0,

		U = 'U',
		R = 'R',
		F = 'F',
		D = 'D',
		L = 'L',
		B = 'B',

		// double layer
		u = 'u',
		r = 'r',
		f = 'f',
		d = 'd',
		l = 'l',
		b = 'b',

		// middle layer
		M = 'M',
		E = 'E',
		S = 'S',

		// whole cube
		x = 'x',
		y = 'y',
		z = 'z',
	} face;
	enum move_direction {
		cw,
		ccw,
		dbl,
	} dir;
};

#define FLIP_DIR(dir_) (dir_ == cw ? ccw : (dir_ == ccw ? cw : dir_))
#define FLIP_FACE(face_) (face_ == F ? B : (face_ == R ? L : (face_ == U ? D : (face_ == B ? F : (face_ == L ? R : (face_ == D ? U : NO_FACE))))))

struct sticker_rotations {
	enum axis {
		AXIS_X = 'x',
		AXIS_Y = 'y',
		AXIS_Z = 'z',
	} axis;
	enum move_direction dir;
	uint64_t stickers;   // bitmask of what stickers to rotate
	uint32_t start_time; // SDL_GetTicks
};

// stores which pieces are moved during a rotation
struct move_map {
	// only one layer rotations
	enum rotation_face {
		NONE = 0,
		FACE_U = 'U',
		FACE_R = 'R',
		FACE_F = 'F',
		FACE_D = 'D',
		FACE_L = 'L',
		FACE_B = 'B',
		FACE_M = 'M',
		FACE_E = 'E',
		FACE_S = 'S',
	} face;
	enum stickers stickers[3];
};

char get_char_move_face(enum move_face);
char get_char_move_direction(enum move_direction);
void make_move(struct cube *cube, struct move move, struct sticker_rotations *animation);
void reset_cube(struct cube *);
intpos get_sticker_index(intpos face_no, intpos sticker_i);
uint64_t get_sticker_bitmask(intpos face_no, intpos sticker_i);
uint64_t get_face_bitmask(intpos face_no);
#endif //RUBIK_H
