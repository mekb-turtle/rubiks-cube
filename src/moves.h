#ifndef MOVES_H
#define MOVES_H
#include "config.h"
#include <stdbool.h>
#include "rubik.h"
// linked list
struct move_list {
	struct move_list_item {
		struct move item;
		struct move_list_item *next;
	} *head, *tail;
	size_t count, last_shuffle_count, shuffle_count;
};
extern struct move_list moves;
extern int_time current_turn_time;
void init_moves();
void free_moves();
bool send_move_unlimited(struct move move);
bool send_move(struct move move);
bool update_moves(int_time current_time, struct cube *cube);
bool shuffle_cube(struct cube *);
void update_turn_time();
#endif
