#include <device.h>
#include <log.h>
#include <opengl.h>
#include <configuration.h>

namespace xs::device::internal
{
}

using namespace xs;

void device::initialize()
{
}

void device::shutdown()
{
}

void device::swap_buffers()
{
}

void device::poll_events()
{
}

bool device::can_close()
{
    return false;
}

bool device::request_close()
{
    return false;
}

bool device::should_close()
{
    return false;
}

int device::get_width()
{
    return 0;
}

int device::get_height()
{
    return 0;
}
