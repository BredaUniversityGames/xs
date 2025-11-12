#include "xs.hpp"

// SDL will handle the main/WinMain conversion automatically
// to handle Console and Windows subsystems
#include <SDL3/SDL_main.h>

extern "C"
{
    // Force Nvidia GPU if available
    _declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

// Single main entry point
int main(int argc, char* argv[])
{
    return xs::dispatch(argc, argv);
}

// Include SDL's main implementation to get WinMain wrapper
#include <SDL3/SDL_main_impl.h>