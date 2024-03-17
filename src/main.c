#include <stdio.h>
#include <stdbool.h>
#include "err.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "config.h"
#include "rubik.h"
#include "render.h"
#include "moves.h"

struct cube cube;

int main(int argc, char *argv[]) {
	int ret = 1;
	bool render_init = false;

	// region SDL initialization
	SDL_Window *window = NULL;
	SDL_GLContext context = NULL;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		errx(1, "SDL_Init: %s", SDL_GetError());

	// Anti-aliasing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

	window = SDL_CreateWindow(WINDOW_TITLE,
	                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                          800, 600,
	                          SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (!window) {
		warnx("SDL_CreateWindow: %s", SDL_GetError());
		goto exit;
	}

	context = SDL_GL_CreateContext(window);
	if (!context) {
		warnx("SDL_GL_CreateContext: %s", SDL_GetError());
		goto exit;
	}
	// endregion

	bool loop = true;

	reset_cube(&cube);

	// Enable vsync
	SDL_GL_SetSwapInterval(1);

	if (!initialize_render()) {
		goto exit;
	}

	render_init = 1;

	init_moves();
	update_cube(&cube);
	update_turn_time();

	SDL_Point window_size;
	SDL_Point render_size;
	SDL_Event event;

	Sint32 mouse_x, mouse_y;
	bool mouse_down = false;

	bool arrow_up = false, arrow_right = false, arrow_down = false, arrow_left = false;

	int_time last_time = SDL_GetTicks();

	while (loop) {
		int_time current_time = SDL_GetTicks();

		SDL_GetWindowSize(window, &window_size.x, &window_size.y);
		if (window_size.x > window_size.y) {
			render_size.x = render_size.y = window_size.y;
		} else {
			render_size.x = render_size.y = window_size.x;
		}

		if (render_size.x < 1) render_size.x = 1;
		if (render_size.y < 1) render_size.y = 1;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					loop = false;
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP: {
					SDL_Keysym sym = event.key.keysym;
					bool is_down = event.type == SDL_KEYDOWN;
					switch (sym.sym) {
						case SDLK_UP:
							arrow_up = is_down;
							break;
						case SDLK_RIGHT:
							arrow_right = is_down;
							break;
						case SDLK_DOWN:
							arrow_down = is_down;
							break;
						case SDLK_LEFT:
							arrow_left = is_down;
							break;
					}
					if (!is_down) break;
					bool prime_rotate = !!(sym.mod & KMOD_SHIFT) != !!(sym.mod & KMOD_CAPS); // if shift is down XOR caps lock is on
					bool double_rotate = sym.mod & KMOD_CTRL;
					bool double_layer = sym.mod & KMOD_ALT;
					if (sym.sym >= 'A' && sym.sym <= 'Z') sym.sym = sym.sym + 'a' - 'A'; // make lowercase
					struct move move;
					move.face = NO_FACE;
					if (double_rotate)
						move.dir = dbl;
					else if (prime_rotate)
						move.dir = ccw;
					else
						move.dir = cw;
					switch (sym.sym) {
						case 'u':
							move.face = double_layer ? u : U;
							break;
						case 'r':
							move.face = double_layer ? r : R;
							break;
						case 'f':
							move.face = double_layer ? f : F;
							break;
						case 'd':
							move.face = double_layer ? d : D;
							break;
						case 'l':
							move.face = double_layer ? l : L;
							break;
						case 'b':
							move.face = double_layer ? b : B;
							break;
						case 'x':
							move.face = x;
							break;
						case 'y':
							move.face = y;
							break;
						case 'z':
							move.face = z;
							break;
						case 'm':
							move.face = M;
							break;
						case 'e':
							move.face = E;
							break;
						case 's':
							move.face = S;
							break;
						case ' ':
							reset_camera();
							break;
						case SDLK_BACKSPACE:
							shuffle_cube(&cube);
							break;
						default:
							break;
					}
					// default because we don't want to move if the user presses an invalid letter
					if (move.face == NO_FACE) break;
					if (!send_move(move)) goto exit;
					break;
				}
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_LEFT)
						mouse_down = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						mouse_down = true;
						mouse_x = event.motion.x;
						mouse_y = event.motion.y;
					}
					break;
				case SDL_MOUSEMOTION:
					if (!mouse_down) break;
					float dx = event.motion.x - mouse_x,
					      dy = event.motion.y - mouse_y;
					dx /= render_size.x;
					dy /= render_size.y;

					const float m = 200;
					dx *= m;
					dy *= m;
					rotate_camera(dx, dy);
					mouse_x = event.motion.x;
					mouse_y = event.motion.y;
					break;
			}
		}

		// rotate the view of the cube using the arrow keys
		float look_x = 0, look_y = 0;
		if (arrow_up) look_y--;
		if (arrow_right) look_x++;
		if (arrow_down) look_y++;
		if (arrow_left) look_x--;
		if (look_x != 0 || look_y != 0) {
			if (look_x != 0 && look_y != 0) {
				look_x *= 0.7071; // 1/sqrt(2)
				look_y *= 0.7071;
			}
			const float multiplier = (current_time - last_time) * 0.15;
			rotate_camera(look_x * multiplier, look_y * multiplier);
		}
		last_time = current_time;

		update_moves(current_time, &cube);

		glViewport((window_size.x - render_size.x) / 2, (window_size.y - render_size.y) / 2, render_size.x, render_size.y);

		render(&cube);

		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			warnx("OpenGL error: %i", error);
			goto exit;
		}

		SDL_GL_SwapWindow(window);
	}

	ret = 0;
exit:
	if (render_init) unload();
	if (context) SDL_GL_DeleteContext(context);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();
	free_moves();

	return ret;
}
