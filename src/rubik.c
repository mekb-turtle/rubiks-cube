#include "./rubik.h"

char get_char_move_face(enum move_face face) {
	return "URFDLBurfdlbMESxyz"[face];
}

char get_char_move_direction(enum move_direction dir) {
	return "\0'2"[dir];
}

#define flip_dir(dir_) (dir_ == cw ? ccw : dir_ == ccw ? cw \
	                                                   : dir_)

#define opposite_face(face_) (face_ == FACE_U ? FACE_D : face_ == FACE_R ? FACE_L \
	                                             : face_ == FACE_F       ? FACE_B \
	                                             : face_ == FACE_D       ? FACE_U \
	                                             : face_ == FACE_L       ? FACE_R \
	                                             : face_ == FACE_B       ? FACE_F \
	                                                                     : NONE)

static const intpos faces_map[] = {
        [FACE_U] = 0,
        [FACE_F] = 1,
        [FACE_R] = 2,
        [FACE_B] = 3,
        [FACE_L] = 4,
        [FACE_D] = 5,
};

// stores which pieces are moved during a rotation
static const struct face_movement {
	enum rotation_face face;
	enum stickers stickers[3];
} face_movement[][4] = {
        {{F, {top_right, top_center, top_left}}, {L, {top_right, top_center, top_left}}, {B, {top_right, top_center, top_left}}, {R, {top_right, top_center, top_left}}},
        {{U, {top_right, top_center, top_left}}, {R, {top_right, top_center, top_left}}, {D, {top_right, top_center, top_left}}, {L, {top_right, top_center, top_left}}},
        {{U, {top_right, top_center, top_left}}, {B, {top_right, top_center, top_left}}, {D, {top_right, top_center, top_left}}, {F, {top_right, top_center, top_left}}},
        {{F, {top_right, top_center, top_left}}, {L, {top_right, top_center, top_left}}, {B, {top_right, top_center, top_left}}, {R, {top_right, top_center, top_left}}},
        {{U, {top_right, top_center, top_left}}, {F, {top_right, top_center, top_left}}, {D, {top_right, top_center, top_left}}, {B, {top_right, top_center, top_left}}},
        {{F, {top_right, top_center, top_left}}, {R, {top_right, top_center, top_left}}, {B, {top_right, top_center, top_left}}, {L, {top_right, top_center, top_left}}}
};

void make_move(struct cube *cube, struct move move) {
	struct base_rotation rotations[3] = {
	        {.face = NONE},
	        {.face = NONE},
	        {.face = NONE},
	};

	// simplify double and whole cube moves to simple 1 layer rotations or middle layer
	switch (move.face) {
		case U:
		case R:
		case F:
		case D:
		case L:
		case B:
		case M:
		case E:
		case S:
			rotations[0].face = move.face;
			rotations[0].dir = move.dir;
			break;

		case u:
		case r:
		case f:
		case d:
		case l:
		case b:
			rotations[0].face = move.face - u + U;
			rotations[0].dir = move.face;
			switch (move.face) {
				case u:
					rotations[1].face = FACE_E;
					rotations[1].dir = flip_dir(move.dir);
				case r:
					rotations[1].face = FACE_M;
					rotations[1].dir = flip_dir(move.dir);
				case f:
					rotations[1].face = FACE_S;
					rotations[1].dir = move.dir;
				case d:
					rotations[1].face = FACE_E;
					rotations[1].dir = move.dir;
				case l:
					rotations[1].face = FACE_M;
					rotations[1].dir = move.dir;
				case b:
					rotations[1].face = FACE_S;
					rotations[1].dir = flip_dir(move.dir);
				default:
					break;
			}
			break;

		case x:
			rotations[0].face = FACE_R;
			rotations[0].dir = move.dir;
			rotations[1].face = FACE_M;
			rotations[1].dir = flip_dir(move.dir);
			rotations[2].face = FACE_L;
			rotations[2].dir = flip_dir(move.dir);
			break;
		case y:
			rotations[0].face = FACE_U;
			rotations[0].dir = move.dir;
			rotations[1].face = FACE_E;
			rotations[1].dir = flip_dir(move.dir);
			rotations[2].face = FACE_D;
			rotations[2].dir = flip_dir(move.dir);
			break;
		case z:
			rotations[0].face = FACE_F;
			rotations[0].dir = move.dir;
			rotations[1].face = FACE_S;
			rotations[1].dir = move.dir;
			rotations[2].face = FACE_B;
			rotations[2].dir = flip_dir(move.dir);
			break;
	}

	intpos times = 1;
	if (move.dir == ccw) times = 3;
	if (move.dir == dbl) times = 2;

	for (intpos i = 0; i < times; ++i) {
		struct base_rotation rotation = rotations[i];
		if (rotation.face == NONE) continue;

		face_color(*stickers)[20];
		intpos sticker_i = 0;

		face_movement[rotation.face];

		intpos face_i = faces_map[rotation.face];

		struct face *rotate_face = &cube->faces[face_i];
		struct cube old_cube = *cube;
		struct face old_face = old_cube.faces[face_i];

		rotate_face->top_right = old_face.top_left;
		rotate_face->middle_right = old_face.top_center;
		rotate_face->bottom_right = old_face.top_right;
		rotate_face->bottom_center = old_face.middle_right;
		rotate_face->bottom_left = old_face.bottom_right;
		rotate_face->middle_left = old_face.bottom_center;
		rotate_face->top_left = old_face.bottom_left;
		rotate_face->top_center = old_face.middle_left;
	}
}

void reset_cube(struct cube *cube) {
	for (intpos face = 0; face < 6; ++face) {
		for (intpos sticker = 0; sticker < 9; ++sticker) {
			cube->faces[face].stickers[sticker] = face;
		}
	}
}

void shuffle_cube(struct cube *cube) {
}
