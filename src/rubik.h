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

struct base_rotation {
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
	enum move_direction dir;
	Uint32 move_time;
};

#define move(face_, dir_) ((struct move){.face = (face_), .dir = (dir_)})
char get_char_move_face(enum move_face);
char get_char_move_direction(enum move_direction);
void make_move(struct cube *cube, struct move move, struct base_rotation (*rotations)[3]);
void reset_cube(struct cube *);
void shuffle_cube(struct cube *);
#endif //RUBIK_H
