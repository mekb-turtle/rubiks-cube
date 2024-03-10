#include "rubik.h"
#include "config.h"

char get_char_move_face(enum move_face face) {
	return "URFDLBurfdlbMESxyz"[face];
}

char get_char_move_direction(enum move_direction dir) {
	return "\0'2"[dir];
}

struct base_rotation {
	enum rotation_face face;
	enum move_direction dir;
	time move_time;
};

#define flip_dir(dir_) (dir_ == cw ? ccw : (dir_ == ccw ? cw : dir_))

const struct move_map moves_map[][4] = {
  // U:
        {{F, {top_right, top_center, top_left}},          {L, {top_right, top_center, top_left}},          {B, {top_right, top_center, top_left}},          {R, {top_right, top_center, top_left}}         },
 // F:
        {{U, {bottom_left, bottom_center, bottom_right}}, {R, {top_left, middle_left, bottom_left}},       {D, {top_right, top_center, top_left}},          {L, {bottom_right, middle_right, top_right}}   },
 // R:
        {{U, {bottom_right, middle_right, top_right}},    {B, {top_left, middle_left, bottom_left}},       {D, {bottom_right, middle_right, top_right}},    {F, {bottom_right, middle_right, top_right}}   },
 // B:
        {{U, {top_right, top_center, top_left}},          {L, {top_left, middle_left, bottom_left}},       {D, {bottom_left, bottom_center, bottom_right}}, {R, {bottom_right, middle_right, top_right}}   },
 // L:
        {{U, {top_left, middle_left, bottom_left}},       {F, {top_left, middle_left, bottom_left}},       {D, {top_left, middle_left, bottom_left}},       {B, {bottom_right, middle_right, top_right}}   },
 // D:
        {{F, {bottom_left, bottom_center, bottom_right}}, {R, {bottom_left, bottom_center, bottom_right}}, {B, {bottom_left, bottom_center, bottom_right}}, {L, {bottom_left, bottom_center, bottom_right}}},
 // M:
        {{U, {top_center, middle_center, bottom_center}}, {F, {top_center, middle_center, bottom_center}}, {D, {top_center, middle_center, bottom_center}}, {B, {bottom_center, middle_center, top_center}}},
 // E:
        {{F, {middle_left, middle_center, middle_right}}, {R, {middle_left, middle_center, middle_right}}, {B, {middle_left, middle_center, middle_right}}, {L, {middle_left, middle_center, middle_right}}},
 // S:
        {{U, {middle_left, middle_center, middle_right}}, {R, {top_center, middle_center, bottom_center}}, {D, {middle_right, middle_center, middle_left}}, {L, {bottom_center, middle_center, top_center}}},
};

const intpos faces_map[] = {
        [FACE_U] = 0,
        [FACE_F] = 1,
        [FACE_R] = 2,
        [FACE_B] = 3,
        [FACE_L] = 4,
        [FACE_D] = 5,
        [FACE_M] = 6,
        [FACE_E] = 7,
        [FACE_S] = 8,
};

void make_move(struct cube *cube, struct move move, struct sticker_rotations *animation) {
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

		// double layer turns use the single face layer + the middle layer
		case u:
		case r:
		case f:
		case d:
		case l:
		case b:
			rotations[0].face = move.face - u + U;
			rotations[0].dir = move.dir;
			switch (move.face) {
				case u:
					rotations[1].face = FACE_E;
					rotations[1].dir = flip_dir(move.dir);
					break;
				case r:
					rotations[1].face = FACE_M;
					rotations[1].dir = flip_dir(move.dir);
					break;
				case f:
					rotations[1].face = FACE_S;
					rotations[1].dir = move.dir;
					break;
				case d:
					rotations[1].face = FACE_E;
					rotations[1].dir = move.dir;
					break;
				case l:
					rotations[1].face = FACE_M;
					rotations[1].dir = move.dir;
					break;
				case b:
					rotations[1].face = FACE_S;
					rotations[1].dir = flip_dir(move.dir);
					break;
				default:
					break;
			}
			break;

		// whole cube rotations just rotate 3 layers
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

		default:
			return;
	}

	if (animation) {
		animation->dir = rotations[0].dir;
		switch (rotations[0].face) {
			case U:
				animation->axis = AXIS_Y;
				animation->dir = flip_dir(animation->dir);
				break;
			case R:
				animation->axis = AXIS_X;
				break;
			case F:
			case S:
				animation->axis = AXIS_Z;
				break;
			case D:
			case E:
				animation->axis = AXIS_Y;
				break;
			case L:
			case M:
				animation->axis = AXIS_X;
				animation->dir = flip_dir(animation->dir);
				break;
			case B:
				animation->axis = AXIS_Z;
				animation->dir = flip_dir(animation->dir);
				break;
			default:
				break;
		}
		animation->stickers = 0;
	}

	for (intpos i = 0; i < 3; ++i) {
		struct base_rotation rotation = rotations[i];
		if (rotation.face == NONE) continue;

		intpos times = 1;
		if (rotation.dir == ccw) times = 3;
		if (rotation.dir == dbl) times = 2;

		for (intpos rotation_counter = times; rotation_counter; --rotation_counter) {
			struct cube old_cube = *cube;

			intpos rotation_face_i = faces_map[rotation.face];

			// store pointers to stickers to rotate
			face_color *stickers[12];
			face_color *stickers_old[12];
			for (intpos face_i = 0, stickers_i = 0; face_i < 4; ++face_i) {
				struct move_map move = moves_map[rotation_face_i][face_i];
				for (intpos face_sticker_i = 0; face_sticker_i < 3; ++face_sticker_i, ++stickers_i) {
					intpos face = faces_map[move.face];
					intpos sticker = move.stickers[face_sticker_i];
					stickers[stickers_i] = &cube->faces[face].stickers[sticker];
					stickers_old[stickers_i] = &old_cube.faces[face].stickers[sticker];
					if (animation) {
						animation->stickers |= get_sticker_bitmask(face, sticker);
					}
				}
			}

			// rotate the stickers
			for (intpos stickers_i = 0; stickers_i < 12; ++stickers_i) {
				intpos stickers_j = (stickers_i + 3) % 12; // rotate stickers by 3 spaces
				*stickers[stickers_j] = *stickers_old[stickers_i];
			}

			if (rotation_face_i < 6) {
				struct face *rotate_face = &cube->faces[rotation_face_i];
				struct face old_face = old_cube.faces[rotation_face_i];

				// bitmask for all 9 stickers
				animation->stickers |= get_face_bitmask(rotation_face_i);

				// rotate top stickers
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
	}

	return;
}

void reset_cube(struct cube *cube) {
	for (intpos face = 0; face < 6; ++face) {
		for (intpos sticker = 0; sticker < 9; ++sticker) {
			cube->faces[face].stickers[sticker] = face;
		}
	}
}

void shuffle_cube(struct cube *cube) {
	// TODO
}

intpos get_sticker_index(intpos face_no, intpos sticker_i) {
	return face_no * 9 + sticker_i;
}

uint64_t get_sticker_bitmask(intpos face_no, intpos sticker_i) {
	return 1ull << get_sticker_index(face_no, sticker_i);
}

uint64_t get_face_bitmask(intpos face_no) {
	return 0777ull << (face_no * 9);
}
