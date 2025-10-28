#include "xs.hpp"

extern "C"
{
    // Force Nvidia GPU if available
    _declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

int main(int argc, char* argv[])
{
    return xs::dispatch(argc, argv);
}