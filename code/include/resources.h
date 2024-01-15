#pragma once

namespace xs
{
    namespace resources
    {
        /*
        * Initialize the resource system
        *
        * This function is responsible for setting up the resource system.
        * It loads a compressed game archive, uncompresses it, populates the
        * global resource pool, and logs information about the loaded content files.
        *
        * @return True if initialization is successful, false otherwise.
        */
        bool initialize();

        /*
        * Shutdown the resource system
        *
        * This function is responsible for cleaning up the resource system.
        * It clears the global resource pool, releasing any allocated resources.
        */
        void shutdown();
    }
}
