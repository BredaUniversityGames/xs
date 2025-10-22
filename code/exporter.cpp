#include "exporter.hpp"
#include "fileio.hpp"
#include "resource_pipeline.hpp"
#include "log.hpp"
#include "types.hpp"
#include "miniz.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

namespace xs
{

namespace fs = std::filesystem;

namespace archive_generator
{
	struct archive
	{
		archive(std::unique_ptr<blob>&& b, size_t s)
			:blob(std::move(b))
			,size(s)
		{}

		std::unique_ptr<blob> blob;
		size_t size;
	};

	// blob_builder is a helper class for building binary blobs.
	class blob_builder
	{
	public:
		// Constructor for blob_builder, takes a pointer to blob and an initial offset.
		blob_builder(blob* blob_data, size_t initial_offset = 0)
			: _blob_data(blob_data)
			, _offset(initial_offset)
		{}

		// Add data to the blob and update the offset
		void add_data(const void* data, size_t size)
		{
			memcpy(_blob_data->data() + _offset, data, size);
			_offset += size;
		}

		// Get the current offset
		size_t offset() const
		{
			return _offset;
		}

	private:
		blob* _blob_data;
		size_t _offset;
	};

	// Calculate the total amount of entries.
	size_t count_total_amount_of_entries(const fs::path& p)
	{
		// Check if the path exists on disk.
		if (fs::exists(p) == false)
		{
			log::error("Path does not exist on disk: {0}", p.string());
			return 0;
		}

		unsigned long count = 0;

		// Iterate through directory entries and calculate total size.
		for (const auto& entry : fs::recursive_directory_iterator(p))
		{
			if (fs::is_directory(entry))
			{
				// Directories do not need to be processed.
				continue;
			}

			if (resource_pipeline::is_supported_file_format(entry.path().extension().string()) == false)
			{
				// Unsupported file format
				continue;
			}

			++count;
		}

		return count;
	}
	
	// Calculate the total amount of entries.
	size_t count_total_amount_of_entries(const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		fs::path root_path = fs::path(root);

		size_t entry_count = 0;

		if (!sub_dirs.empty())
		{
			// Calculate total size based on specified subdirectories.
			for (const std::string& subdir : sub_dirs)
			{
				fs::path subdir_path = fs::path(subdir);
				fs::path full_path = root_path / subdir_path;

				entry_count += count_total_amount_of_entries(full_path);
			}
		}
		else
		{
			// Calculate total size for the entire root path.
			entry_count = count_total_amount_of_entries(root_path);
		}

		return entry_count;
	}

	// Calculate the total size of content entries in the specified directory.
	size_t calculate_content_entries_size(const fs::path& p)
	{
		// Check if the path exists on disk.
		if (fs::exists(p) == false)
		{
			log::error("Path does not exist on disk: {0}", p.string());
			return 0;
		}

		unsigned long total_memory = 0;
		unsigned long cook_header_size = sizeof(resource_pipeline::content_header);

		// Iterate through directory entries and calculate total size.
		for (const auto& entry : fs::recursive_directory_iterator(p))
		{
			if (fs::is_directory(entry))
			{
				// Directories do not need to be processed.
				continue;
			}

			std::string extension = entry.path().extension().string();
			if (resource_pipeline::is_supported_file_format(extension) == false)
			{
				// Unsupported file format
				continue;
			}

			unsigned long prev = total_memory;

			// Add content_header size and file size to total_memory.
			total_memory += cook_header_size;

			if (resource_pipeline::is_text_file(extension))
			{
				// Text files will be compressed
				unsigned long src_len = (unsigned long)entry.file_size();
				unsigned long cmp_len = compressBound(src_len);

				total_memory += cmp_len;
			}
			else
			{
				// Binary files will not be compressed
				unsigned long src_len = (unsigned long)entry.file_size();

				total_memory += src_len;
			}

			// Check for size_t overflow.
			assert(prev < total_memory && "size_t overflow, we should split the archive if this turns out to be the case");
		}

		return total_memory;
	}

	// Calculate the total size of content entries in the specified directory.
	size_t calculate_content_entries_size(const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		fs::path root_path = fs::path(root);

		size_t total_size = 0;

		if (!sub_dirs.empty())
		{
			// Calculate total size based on specified subdirectories.
			for (const std::string& subdir : sub_dirs)
			{
				fs::path subdir_path = fs::path(subdir);
				fs::path full_path = root_path / subdir_path;

				total_size += calculate_content_entries_size(full_path);
			}
		}
		else
		{
			// Calculate total size for the entire root path.
			total_size = calculate_content_entries_size(root_path);
		}

		return total_size;
	}
		
	
	// Add content entries to the blob_builder.
	void add_content_entries(blob_builder* builder, const fs::path& p)
	{
		// Check if the path exists on disk.
		if (fs::exists(p) == false)
		{
			log::error("Path does not exist on disk: {0}", p.string());
			return;
		}

		// Iterate through directory entries and add valid content entries.
		for (auto& entry : fs::recursive_directory_iterator(p))
		{
			if (fs::is_directory(entry))
			{
				// Directories do not need to be processed.
				continue;
			}

			fs::path entry_path = entry.path();

			std::string s_entry_path = entry_path.string();
			std::string s_entry_path_extension = entry_path.extension().string();

			if (resource_pipeline::is_supported_file_format(s_entry_path_extension) == false)
			{
				// Unsupported file format.
				continue;
			}

			log::info("Allocating space for file entry: {0}", entry_path.filename().string());

			// Each file entry has a header to store file path, file offset in the archive and file size.
			unsigned long src_len = static_cast<unsigned long>(entry.file_size());
			unsigned long cmp_len = static_cast<unsigned long>(resource_pipeline::is_text_file(s_entry_path_extension) ? compressBound(src_len) : 0);

			// The actual data of the content file
			blob f = fileio::read_binary_file(s_entry_path);

			if (cmp_len != 0)
			{
				blob data;
				data.reserve(cmp_len);

				// Compress the data.
				int cmp_status = compress((unsigned char*)data.data(), &cmp_len, (const unsigned char*)f.data(), src_len);
				if (cmp_status != Z_OK)
				{
					log::error("compression of {0} failed!", s_entry_path);
					continue;
				}

				// We first have to compress to know the actual compressed size of the content
				resource_pipeline::content_header h(s_entry_path, builder->offset() + sizeof(resource_pipeline::content_header), src_len, cmp_len);

				log::info("Content Header:");
				log::info("\tPath: {}", s_entry_path);
				log::info("\tFile offset: {}", builder->offset() + sizeof(resource_pipeline::content_header));
				log::info("\tFile Size: {}", src_len);
				log::info("\tFile Size Compressed: {}", cmp_len);

				builder->add_data(&h, sizeof(resource_pipeline::content_header));
				builder->add_data(data.data(), cmp_len);
			}
			else
			{
				resource_pipeline::content_header h(s_entry_path, builder->offset() + sizeof(resource_pipeline::content_header), src_len, cmp_len);

				log::info("Content Header:");
				log::info("\tPath: {}", s_entry_path);
				log::info("\tFile offset: {}", builder->offset() + sizeof(resource_pipeline::content_header));
				log::info("\tFile Size: {}", src_len);
				log::info("\tFile Size Compressed: {}", cmp_len);

				builder->add_data(&h, sizeof(resource_pipeline::content_header));
				builder->add_data(f.data(), entry.file_size());
			}
		}
	}

	// Add content entries to the blob_builder.
	void add_content_entries(blob_builder* builder, const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		fs::path root_path = fs::path(root);

		// Populate the blob with content entries.
		if (!sub_dirs.empty())
		{
			for (const std::string& subdir : sub_dirs)
			{
				fs::path subdir_path = fs::path(subdir);
				fs::path full_path = root_path / subdir_path;

				add_content_entries(builder, full_path);
			}
		}
		else
		{
			add_content_entries(builder, root_path);
		}
	}

	// Generate an archive for the specified root and subdirectories.
	archive generate(const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		size_t entry_count = count_total_amount_of_entries(root, sub_dirs);

		log::info("Calculated amount of entries for cooking process: {}", entry_count);

		size_t total_size = 0;

		total_size += sizeof(size_t); // We will store the entry count as well so we will have to allocate space for this
		total_size += calculate_content_entries_size(root, sub_dirs);

 		log::info("Calculated content size for cooking process: {} bytes", total_size);

		// Create the archive with the calculated total size.
		std::unique_ptr<blob> blob = std::make_unique<xs::blob>();
		blob->reserve(total_size);

		blob_builder builder = blob_builder(blob.get());

		// Store the amount of entries within the given directories
		builder.add_data(&entry_count, sizeof(size_t));
		// Store all the content entries within the given directories
		add_content_entries(&builder, root, sub_dirs);

		// Check for memory allocation issues.
		if (total_size != builder.offset())
		{
			log::info("Allocated more memory than what was actually stored, allocated: {0}, stored: {1}", total_size, builder.offset());
		}

		return archive(std::move(blob), builder.offset());
	}
}

// ------------------------------------------------------------------------
// New archive format using cereal for cross-platform serialization
// ------------------------------------------------------------------------
namespace archive_v2
{
	struct ContentEntry
	{
		std::string relative_path;       // Relative path from content root
		uint64_t uncompressed_size;      // Original file size
		std::vector<std::byte> data;     // File data (compressed if is_compressed=true)
		bool is_compressed;              // Whether data is zlib compressed

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(relative_path, uncompressed_size, data, is_compressed);
		}
	};

	struct ArchiveData
	{
		std::vector<ContentEntry> entries;

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(entries);
		}
	};
}

namespace exporter
{
	size_t write_data_to_archive(std::ofstream& stream, const char* data, size_t size)
	{
		stream.write(data, size);
		return size;
	}

	// Write the compressed archive to a file.
	bool write_archive(const std::string& output_path, const archive_generator::archive& a)
	{
		std::ofstream ofs;
		ofs.open(output_path, std::ios::out | std::ios::trunc | std::ios::binary);

		// Check if the file is successfully opened.
		if (ofs.is_open())
		{
			size_t total_size = write_data_to_archive(ofs, reinterpret_cast<const char*>(a.blob->data()), a.size * sizeof(unsigned char));

			ofs.close();

			log::info("Written ({0} bytes) archive to disk {1}", total_size, output_path);

			return true;
		}

		log::error("Failed to write {0} to disk", output_path);

		return false;
	}

	// Export content by generating and compressing an archive.
	bool export_content(const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		archive_generator::archive a = archive_generator::generate(root, sub_dirs);

		// Write the compressed archive to a file.
		return write_archive(resource_pipeline::make_archive_path(root, sub_dirs), a);
	}

	// Export content by generating and compressing an archive to a specific path.
	bool export_content_to_path(const std::string& root, const std::vector<std::string>& sub_dirs, const std::string& output_path)
	{
		archive_generator::archive a = archive_generator::generate(root, sub_dirs);

		// Write the compressed archive to the specified path.
		return write_archive(output_path, a);
	}

	// ------------------------------------------------------------------------
	// V2 Archive Export - Simplified cross-platform export using cereal
	// ------------------------------------------------------------------------
	bool export_archive(const std::vector<std::string>& source_dirs, const std::string& output_path)
	{
		archive_v2::ArchiveData archive;

		// Process each source directory
		for (const auto& source_dir_str : source_dirs)
		{
			fs::path source_dir = fs::path(source_dir_str);

			if (!fs::exists(source_dir))
			{
				log::error("Source directory does not exist: {}", source_dir_str);
				continue;
			}

			log::info("Processing directory: {}", source_dir_str);

			// Single-pass iteration: read and process all files
			for (const auto& entry : fs::recursive_directory_iterator(source_dir))
			{
				if (!fs::is_regular_file(entry))
					continue;

				std::string extension = entry.path().extension().string();
				if (!resource_pipeline::is_supported_file_format(extension))
					continue;

				archive_v2::ContentEntry content;
				content.relative_path = fs::relative(entry.path(), source_dir).string();
				content.uncompressed_size = entry.file_size();

				// Normalize path separators to forward slashes for cross-platform compatibility
				for (char& c : content.relative_path)
				{
					if (c == '\\')
						c = '/';
				}

				// Read file data
				blob file_data = fileio::read_binary_file(entry.path().string());

				// Compress text files
				if (resource_pipeline::is_text_file(extension))
				{
					unsigned long src_len = static_cast<unsigned long>(file_data.size());
					unsigned long compressed_size = compressBound(src_len);

					content.data.resize(compressed_size);

					int result = compress(
						reinterpret_cast<unsigned char*>(content.data.data()), &compressed_size,
						reinterpret_cast<const unsigned char*>(file_data.data()), src_len);

					if (result != Z_OK)
					{
						log::error("Failed to compress {}", content.relative_path);
						continue;
					}

					content.data.resize(compressed_size);
					content.is_compressed = true;

					log::info("Packed (compressed): {} ({} -> {} bytes)",
						content.relative_path, src_len, compressed_size);
				}
				else
				{
					// Binary files are stored uncompressed
					content.data = std::move(file_data);
					content.is_compressed = false;

					log::info("Packed: {} ({} bytes)",
						content.relative_path, content.data.size());
				}

				archive.entries.push_back(std::move(content));
			}
		}

		// Write archive using cereal
		try
		{
			std::ofstream ofs(output_path, std::ios::binary);
			if (!ofs)
			{
				log::error("Failed to open output file: {}", output_path);
				return false;
			}

			cereal::BinaryOutputArchive cereal_archive(ofs);
			cereal_archive(archive);

			log::info("Successfully wrote archive with {} entries to: {}",
				archive.entries.size(), output_path);

			return true;
		}
		catch (const std::exception& e)
		{
			log::error("Failed to write archive: {}", e.what());
			return false;
		}
	}
}

}
