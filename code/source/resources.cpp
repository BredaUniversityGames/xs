#include "resources.h"
#include "resource_pipeline.h"
#include "fileio.h"
#include "log.h"

#include "miniz/miniz.h"

#include <unordered_map>
#include <filesystem>
#include <memory>

namespace xs::resources
{
    // Alias for std::filesystem
    namespace fs = std::filesystem;

    // ------------------------------------------------------------------------
    // Structure representing an element within an archive
    struct ArchiveElement
    {
        resource_pipeline::ContentHeader header;
        Blob content;
    };

    // ------------------------------------------------------------------------
    // Class responsible for parsing an archive
    class ArchiveParser
    {
    public:
        // ------------------------------------------------------------------------
        // Constructor taking a pointer to an archive
        ArchiveParser(const resource_pipeline::Archive* archive)
            :_archive(archive)
            ,_total_read(0)
        {}

        // ------------------------------------------------------------------------
        // Check if there is more data to be read in the archive
        bool peek() const
        {
            return _total_read < _archive->src_len;
        }

        // ------------------------------------------------------------------------
        // Get the next element from the archive
        ArchiveElement next()
        {
            assert(_total_read < _archive->src_len);

            ArchiveElement archive_element;

            // Copy content header from archive data
            memcpy(&archive_element.header, _archive->data.data() + _total_read, sizeof(resource_pipeline::ContentHeader));

            // Log information about the decompression
            log::info("Decompressing file entry: {0}", fs::path(archive_element.header.file_path).filename().string());
            log::info("\tFile Path: {}", archive_element.header.file_path);
            log::info("\tFile Size: {} bytes", archive_element.header.file_size);
            log::info("\tIs Text: {}", archive_element.header.is_text);

            // Copy content data from archive
            Blob content;
            content.reserve(archive_element.header.file_size);
            memcpy(content.data(), _archive->data.data() + sizeof(resource_pipeline::ContentHeader) + _total_read, archive_element.header.file_size);

            // Update the total read counter
            _total_read += static_cast<ulong>(sizeof(resource_pipeline::ContentHeader));
            _total_read += static_cast<ulong>(archive_element.header.file_size);

            return archive_element;
        }
            
    private:
        const resource_pipeline::Archive* _archive;
        ulong _total_read;
    };

    // ------------------------------------------------------------------------
    // Class representing a pool of resources
    class ResourcePool
    {
    public:
        // ------------------------------------------------------------------------
        // Add a resource to the pool
        void add_binary(const std::string& filepath, Blob&& blob)
        {
            if (_binary_resources.find(filepath) == _binary_resources.cend())
            {
                _binary_resources.emplace(filepath, std::move(blob));
            }
        }

        // ------------------------------------------------------------------------
        // Add a text resource to the pool
        void add_text(const std::string& filepath, Blob&& blob)
        {
            if (_text_resources.find(filepath) == _text_resources.cend())
            {
                std::string str;
                
                str.reserve(blob.size());

                std::transform(blob.begin(), blob.end(), std::back_inserter(str),
                    [](std::byte b) 
                {
                    return static_cast<char>(b); 
                });

                _text_resources.emplace(filepath, str);
            }
        }

        // ------------------------------------------------------------------------
        // Get a resource from the pool by filepath
        const Blob& get_binary(const std::string& filepath) const
        {
            if (_binary_resources.find(filepath) != _binary_resources.cend())
            {
                return _binary_resources.at(filepath);
            }

            return EMPTY_BINARY;
        }

        // ------------------------------------------------------------------------
        // Get a resource from the pool by filepath
        const std::string& get_text(const std::string& filepath) const
        {
            if (_text_resources.find(filepath) != _text_resources.cend())
            {
                return _text_resources.at(filepath);
            }

            return EMPTY_TEXT;
        }

        // ------------------------------------------------------------------------
        // Has a specific resource available
        bool has_binary(const std::string& filepath) const
        {
            return _binary_resources.find(filepath) != _binary_resources.cend();
        }

        // ------------------------------------------------------------------------
        // Has a specific resource available
        bool has_text(const std::string& filepath) const
        {
            return _text_resources.find(filepath) != _text_resources.cend();
        }

        // ------------------------------------------------------------------------
        // Clear all resources from the pool
        void clear()
        {
            _binary_resources.clear();
            _text_resources.clear();
        }

        // ------------------------------------------------------------------------
        // Get the number of resources in the pool
        size_t count() const
        {
            return _binary_resources.size() + _text_resources.size();
        }

    private:
        static Blob EMPTY_BINARY;
        static std::string EMPTY_TEXT;

        std::unordered_map<std::string, Blob> _binary_resources;
        std::unordered_map<std::string, std::string> _text_resources;
    };

    Blob ResourcePool::EMPTY_BINARY = {};
    std::string ResourcePool::EMPTY_TEXT = {};

    // Global resource pool instance
    ResourcePool g_resources;

    // ------------------------------------------------------------------------
    // Populate the global resource pool from an archive
    void populate_resource_pool(const resource_pipeline::Archive& archive)
    {
        ArchiveParser parser(&archive);
        while(parser.peek())
        {
            ArchiveElement element = parser.next();

            // Add the decompressed content to the global resource pool
            if (element.header.is_text)
            {
                g_resources.add_text(element.header.file_path, std::move(element.content));
            }
            else
            {
                g_resources.add_binary(element.header.file_path, std::move(element.content));
            }
        }
    }

    // ------------------------------------------------------------------------
    // Uncompress a compressed archive
    resource_pipeline::Archive uncompress_archive(const resource_pipeline::CompressedArchive& compressedArchive)
    {
        resource_pipeline::Archive archive(compressedArchive.src_len);

        // Decompress using miniz library
        s32 dcmp_status = uncompress((u8*)archive.data.data(), &archive.src_len, (const u8*)(compressedArchive.data.data()), (mz_ulong)(compressedArchive.cmp_len));
        if (dcmp_status != Z_OK)
        {
            log::error("uncompress failed!");
            return {};
        }

        return archive;
    }

    // ------------------------------------------------------------------------
    // Load game content from a file
    Blob load_game_content()
    {
        // Read game content from a text file
        auto game_str = fileio::read_text_file("[games]/.ini");
        if (game_str.empty())
        {
            log::error("Cannot load game, .ini file seems to be empty");
            return {};
        }

        // Create archive path from game content
        std::string archive_path = resource_pipeline::make_archive_path(fileio::get_path("[games]"), { game_str });

        log::info("Loading archive: {}", archive_path);

        // Read binary content from the created archive path
        return fileio::read_binary_file(archive_path);
    }

    // ------------------------------------------------------------------------
    // Load compressed game archive
    resource_pipeline::CompressedArchive load_game_compressed_archive()
    {
        // Load game content from file
        Blob game_content = load_game_content();

        if (game_content.empty())
        {
            return {};
        }

        log::info("Read ({0} bytes) archive from disk", game_content.size());

        // Create compressed archive structure
        resource_pipeline::CompressedArchive compressed_archive;

        // Read total size of uncompressed data from the content
        size_t total_size = 0;
        memcpy(&compressed_archive.src_len, game_content.data(), sizeof(size_t));

        log::info("Total size of the uncompressed data within archive: {0} bytes", compressed_archive.src_len);

        // Set compressed length taking into account the size of the first value
        compressed_archive.cmp_len = (ulong)(game_content.size() - sizeof(size_t));

        // Read compressed game content and insert into the compressed archive
        auto game_content_start = game_content.begin() + sizeof(size_t);
        auto game_content_end = game_content.end();

        compressed_archive.data.reserve(compressed_archive.cmp_len);
        compressed_archive.data.insert(compressed_archive.data.end(), game_content_start, game_content_end);

        return compressed_archive;
    }

    // ------------------------------------------------------------------------
    // Initialize the resource system
    bool initialize()
    {
        // Load compressed game archive
        resource_pipeline::CompressedArchive compressed_archive = load_game_compressed_archive();

        if (!compressed_archive)
        {
            return false;
        }

        // Uncompress the archive
        resource_pipeline::Archive uncompressed_archive = uncompress_archive(compressed_archive);

        if (!uncompressed_archive)
        {
            return false;
        }

        // Populate the global resource pool
        populate_resource_pool(uncompressed_archive);

        log::info("Loaded {0} content files", g_resources.count());

        return true;
    }

    // ------------------------------------------------------------------------
    // Shutdown the resource system
    void shutdown()
    {
        // Clear the global resource pool
        g_resources.clear();
    }

    // ------------------------------------------------------------------------
    // Check availability for a binary resource
    const Blob& get_binary_resource(const std::string& filename)
    {
        const auto path = fileio::get_path(filename);

        assert(g_resources.has_binary(path));

        return g_resources.get_binary(path);
    }

    // ------------------------------------------------------------------------
    // Check availability for a text resource
    const std::string& get_text_resource(const std::string& filename)
    {
        const auto path = fileio::get_path(filename);

        assert(g_resources.has_text(path));

        return g_resources.get_text(path);
    }
}