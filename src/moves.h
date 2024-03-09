#include "config.h"
#include <stdbool.h>
#include "rubik.h"
void init_moves();
void free_moves();
bool send_move(struct move move);
bool update_moves(time current_time, struct cube *cube);
