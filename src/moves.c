#define MOVES
#include <stdio.h>
#include <time.h>
#include "moves.h"
#include "err.h"
#include "render.h"

struct move_list moves;
int_time current_turn_time;

void init_moves() {
	moves.count = 0;
	moves.shuffle_count = 0;
	moves.head = NULL;
	moves.tail = NULL;
}

static struct move shift_moves() {
	// removes first item from linked list and returns it
	if (moves.head == moves.tail) moves.tail = NULL;
	struct move current = moves.head->item;
	struct move_list_item *next = moves.head->next;
	free(moves.head);
	moves.head = next;

	--moves.count;

	// reset speed for turning
	if (moves.last_shuffle_count > 0 && moves.shuffle_count == 0) update_turn_time();

	moves.last_shuffle_count = moves.shuffle_count;

	// decrease counter for queued shuffle moves
	if (moves.shuffle_count > 0) {
		--moves.shuffle_count;
	}

	return current;
}

void free_moves() {
	// frees every item in the linked list
	while (moves.head) {
		shift_moves();
	}
	moves.count = 0;
	moves.shuffle_count = 0;
}

bool send_move_unlimited(struct move move) {
	// appends the move to the linked list

	struct move_list_item *current = malloc(sizeof(struct move_list_item));
	if (!current) {
		warn("Failed to allocate memory for the move");
		return false;
	}
	current->item = move;
	current->next = NULL;
	++moves.count;
	if (moves.tail) {
		moves.tail->next = current;
	} else {
		moves.head = current;
	}
	moves.tail = current;
	return true;
}

bool send_move(struct move move) {
	if (moves.count >= max_moves) {
		// max moves at a time
		return true;
	}

	return send_move_unlimited(move);
}

static int_time last_moved = 0;

bool update_moves(int_time current_time, struct cube *cube) {
	if (!moves.head) return true;
	if (current_time <= last_moved) return true;

	last_moved = current_time + current_turn_time;
	struct sticker_rotations animation;
	make_move(cube, shift_moves(), &animation);
	animation.start_time = current_time;

	if (!update_cube(cube)) return false;
	if (!send_animation(animation)) return false;

	return true;
}

void update_turn_time() {
	current_turn_time = moves.shuffle_count > 0 ? turn_time_shuffle : turn_time;
	update_render_turn_time();
}

bool shuffle_cube(struct cube *cube) {
	static bool set_seed = false;
	if (!set_seed) {
		srand(time(NULL));
		set_seed = true;
	}
	const enum move_face possible_faces[] = {U, R, F, D, L, B};
	const size_t possible_faces_count = sizeof(possible_faces) / sizeof(possible_faces[0]);
	const enum move_face possible_dirs[] = {cw, ccw, dbl};
	const size_t possible_dirs_count = sizeof(possible_dirs) / sizeof(possible_dirs[0]);
	if (moves.count != 0) return true;
	enum move_face second_last_move = 0;
	enum move_face last_move = 0;
	// don't shuffle if already moving
	for (size_t times = 0; times < 20; ++times) {
		struct move move;
		do {
			move.face = possible_faces[rand() % possible_faces_count];
		} while (move.face == last_move || move.face == FLIP_FACE(second_last_move)); // prevent undoing the move
		move.dir = possible_dirs[rand() % possible_dirs_count];
		second_last_move = last_move;
		last_move = move.face;
		if (!send_move_unlimited(move)) return false;
	}
	moves.shuffle_count = moves.count;
	// update speed for turning
	update_turn_time();
	return true;
}
