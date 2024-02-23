#ifndef RUBIK_H
#define RUBIK_H
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef uint8_t intpos;
typedef uint8_t face_color;

struct cube {
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

struct sticker_rotations {
	enum axis {
		AXIS_X = 'x',
		AXIS_Y = 'y',
		AXIS_Z = 'z',
	} axis;
	enum move_direction dir;
	uint64_t stickers; // bitmask of what stickers to rotate
		Uint32 start_time; // SDL_GetTicks
};

#define move(face_, dir_) ((struct move){.face = (face_), .dir = (dir_)})

char get_char_move_face(enum move_face);
char get_char_move_direction(enum move_direction);
void make_move(struct cube *cube, struct move move, struct sticker_rotations *animation);
void reset_cube(struct cube *);
void shuffle_cube(struct cube *);
intpos get_sticker_bitmask(intpos face_no, intpos sticker_i);
intpos get_face_bitmask(intpos face_no);
#endif //RUBIK_H
