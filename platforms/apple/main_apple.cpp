#include "xs.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Include the SDL main implementation first (provides the real main() for the platform)
#include <SDL3/SDL_main_impl.h>

// Now define our main function - it will be renamed to SDL_main by the macro above
// SDL_main_impl.h provides the actual main() that calls SDL_main via SDL_RunApp
int main(int argc, char* argv[])
{
    return xs::dispatch(argc, argv);
}
