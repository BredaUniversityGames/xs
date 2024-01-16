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

        /*
        * Reads binary data from a file and returns it as a Blob
        *
        * This function reads binary data from the specified file and returns it
        * encapsulated in a Blob object. If the file is not found or cannot be
        * opened, an empty Blob is returned.
        *
        * @param filename - The name of the file to be read.
        *
        * @return The Blob containing the binary data, or an empty Blob if the
        *         operation is unsuccessful.
        */
        const Blob& get_binary_resource(const std::string& filename);

        /*
        * Reads text data from a file and returns it as a string
        *
        * This function reads binary data from the specified file and returns it
        * encapsulated in a Blob object. If the file is not found or cannot be
        * opened, an empty Blob is returned.
        *
        * @param filename - The name of the file to be read.
        *
        * @return The Blob containing the binary data, or an empty Blob if the
        *         operation is unsuccessful.
        */
        const std::string& get_binary_resource(const std::string& filename);
    }
}
