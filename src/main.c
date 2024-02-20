#include <stdio.h>
#include <err.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "./config.h"
#include "./rubik.h"
#include "render.h"

struct cube cube;

int main(int argc, char *argv[]) {
	int ret = 1;

	// region SDL initialization
	SDL_Window *window = NULL;
	SDL_GLContext context = NULL;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		errx(1, "SDL_Init: %s", SDL_GetError());

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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	SDL_Point window_size;
	SDL_Point render_size;
	SDL_Event event;

	while (loop) {
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
							default:
								goto continue_;
						}
						make_move(&cube, move);
					continue_:
					}
					break;
				}
			}
		}

		SDL_GetWindowSize(window, &window_size.x, &window_size.y);
		if (window_size.x > window_size.y) {
			render_size.x = window_size.y;
			render_size.y = window_size.y;
		} else {
			render_size.x = window_size.x;
			render_size.y = window_size.x;
		}
		glViewport((window_size.x - render_size.x) / 2, (window_size.y - render_size.y) / 2, render_size.x, render_size.y);

		glClear(GL_COLOR_BUFFER_BIT);

		render(&cube);

		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			warnx("OpenGL error: %i", error);
			return true;
		}

		SDL_GL_SwapWindow(window);
	}

	ret = 0;
exit:
	if (context) SDL_GL_DeleteContext(context);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();

	return ret;
}
