#define MOVES
#include <stdio.h>
#include "moves.h"
#include "err.h"
#include "render.h"

// linked list
static struct {
	struct move_list {
		struct move item;
		struct move_list *next;
	} *head, *tail;
	size_t count;
} moves;

void init_moves() {
	moves.count = 0;
	moves.head = NULL;
	moves.tail = NULL;
}

static struct move shift_moves() {
	// removes first item from linked list and returns it
	if (moves.head == moves.tail) moves.tail = NULL;
	struct move current = moves.head->item;
	struct move_list *next = moves.head->next;
	free(moves.head);
	moves.head = next;
	--moves.count;
	return current;
}

void free_moves() {
	// frees every item in the linked list
	while (moves.head) {
		shift_moves();
	}
	moves.count = 0;
}

bool send_move(struct move move) {
	if (moves.count >= max_moves) {
		// max moves at a time
		return true;
	}

	// appends the move to the linked list

	struct move_list *current = malloc(sizeof(struct move_list));
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

static time last_moved = 0;

bool update_moves(time current_time, struct cube *cube) {
	if (moves.head) {
		if (current_time <= last_moved) return true;
		last_moved = current_time + turn_time;
		struct sticker_rotations animation;
		make_move(cube, shift_moves(), &animation);
		animation.start_time = current_time;
		if (!update_cube(cube)) return false;
		if (!send_animation(animation)) return false;
	}
	return true;
}
