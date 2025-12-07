#pragma once

struct SDL_Window;
struct SDL_GLContextState;

namespace xs::device
{
	SDL_Window* get_window();
	SDL_GLContextState* get_context();
}
