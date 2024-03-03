#include <stdio.h>
#include <stdbool.h>
#include "err.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "./config.h"
#include "./rubik.h"
#include "render.h"

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

	update_cube(&cube);

	SDL_Point window_size;
	SDL_Point render_size;
	SDL_Event event;

	Sint32 mouse_x, mouse_y;
	bool mouse_down = false;

	while (loop) {
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
				case SDL_TEXTINPUT: {
					char *text = event.text.text;
					// loop character
					for (; *text; ++text) {
						struct move move;
						if (*text >= 'a' && *text <= 'z') {
							move.dir = cw;
							*text = *text + 'A' - 'a';
						} else {
							move.dir = ccw;
						}
						bool invalid = false;
						switch (*text) {
							case 'U':
								move.face = U;
								break;
							case 'R':
								move.face = R;
								break;
							case 'F':
								move.face = F;
								break;
							case 'D':
								move.face = D;
								break;
							case 'L':
								move.face = L;
								break;
							case 'B':
								move.face = B;
								break;
							case 'X':
								move.face = x;
								break;
							case 'Y':
								move.face = y;
								break;
							case 'Z':
								move.face = z;
								break;
							case 'M':
								move.face = M;
								break;
							case 'E':
								move.face = E;
								break;
							case 'S':
								move.face = S;
								break;
							case ' ':
								reset_camera();
								break;
							default:
								invalid = true;
								break;
						}
						if (invalid) continue;
						struct sticker_rotations animation;
						make_move(&cube, move, &animation);
						animation.start_time = SDL_GetTicks();
						send_animation(animation);
						update_cube(&cube);
					}
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

	return ret;
}
