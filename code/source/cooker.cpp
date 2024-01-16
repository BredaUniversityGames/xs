#include "cooker.h"
#include "fileio.h"
#include "resource_pipeline.h"
#include "log.h"
#include "types.h"
#include "miniz/miniz.h"
#include <filesystem>
#include <fstream>

namespace xs
{
	namespace fs = std::filesystem;

	// ------------------------------------------------------------------------
	// This namespace contains functionality for generating archives.
	namespace archive_generator
	{
		// ------------------------------------------------------------------------
		// BlobBuilder is a helper class for building binary blobs.
		class BlobBuilder
		{
		public:
			// ------------------------------------------------------------------------
			// Constructor for BlobBuilder, takes a pointer to Blob and an initial offset.
			BlobBuilder(Blob* blobData, size_t initialOffset = 0)
				: _blob_data(blobData)
				, _offset(initialOffset)
			{}

			// ------------------------------------------------------------------------
			// Add data to the blob and update the offset
			void add_data(const void* data, size_t size)
			{
				memcpy(_blob_data->data() + _offset, data, size);
				_offset += size;
			}

			// ------------------------------------------------------------------------
			// Get the current offset
			size_t offset() const
			{
				return _offset;
			}

		private:
			Blob* _blob_data;
			size_t _offset;
		};

		// ------------------------------------------------------------------------
		// Calculate the total size of content entries in the specified directory.
		ulong calculate_content_entries_size(const fs::path& p)
		{
			// Check if the path exists on disk.
			if (fs::exists(p) == false)
			{
				log::error("Path does not exist on disk: {0}", p.string());
				return 0;
			}

			ulong total_memory = 0;
			ulong cook_header_size = sizeof(resource_pipeline::ContentHeader);

			// Iterate through directory entries and calculate total size.
			for (const auto& entry : fs::recursive_directory_iterator(p))
			{
				// Directories do not need to be processed.
				if (fs::is_directory(entry))
				{
					continue;
				}

				// Unsupported file format
				if (resource_pipeline::is_supported_file_format(entry.path().extension().string()) == false)
				{
					continue;
				}

				ulong prev = total_memory;

				// Add CookHeader size and file size to total_memory.
				total_memory += cook_header_size;
				total_memory += static_cast<ulong>(entry.file_size());

				// Check for size_t overflow.
				assert(prev < total_memory && "size_t overflow, we should split the archive if this turns out to be the case");
			}

			return total_memory;
		}

		// ------------------------------------------------------------------------
		// Add content entries to the BlobBuilder.
		void add_content_entries(BlobBuilder* builder, const fs::path& p)
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
				// Directories do not need to be processed.
				if (fs::is_directory(entry))
				{
					continue;
				}

				// Unsupported file format.
				std::string extension = entry.path().extension().string();
				if (resource_pipeline::is_supported_file_format(extension) == false)
				{
					continue;
				}

				log::info("Compressing file entry: {0}", entry.path().filename().string());

				// Each file entry has a header to store file path and file size.
				resource_pipeline::ContentHeader h(entry.path().string(), entry.file_size(), (char8)resource_pipeline::is_text_file(extension));
				builder->add_data(&h, sizeof(resource_pipeline::ContentHeader));

				// The actual data of the content file
				Blob f = fileio::read_binary_file(entry.path().string());
				builder->add_data(f.data(), h.file_size);
			}
		}

		// ------------------------------------------------------------------------
		// Generate an archive for the specified root and subdirectories.
		resource_pipeline::Archive generate(const std::string& root, const std::vector<std::string>& subDirs)
		{
			fs::path root_path = fs::path(root);

			ulong total_size = 0;

			// Calculate total size based on specified subdirectories.
			if (!subDirs.empty())
			{
				for (const std::string& subdir : subDirs)
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

			log::info("Calculated content size for cooking process: {} bytes", total_size);

			// Create the archive with the calculated total size.
			resource_pipeline::Archive archive(total_size);

			// Reserve memory in BlobBuilder for building the archive.
			BlobBuilder builder = BlobBuilder(&archive.data);

			// Populate the blob with content entries.
			if (!subDirs.empty())
			{
				for (const std::string& subdir : subDirs)
				{
					fs::path subdir_path = fs::path(subdir);
					fs::path full_path = root_path / subdir_path;

					add_content_entries(&builder, full_path);
				}
			}
			else
			{
				add_content_entries(&builder, root_path);
			}

			// Check for memory allocation issues.
			if (total_size != builder.offset())
			{
				log::warn("Allocated more memory than what was actually stored, allocated: {0}, stored: {1}", total_size, builder.offset());
			}

			return archive;
		}
	}

	namespace cooker
	{
		// ------------------------------------------------------------------------
		// Compress an archive using zlib compression.
		resource_pipeline::CompressedArchive compress_archive(const resource_pipeline::Archive& a)
		{
			resource_pipeline::CompressedArchive compressed_archive;

			compressed_archive.src_len = (mz_ulong)a.src_len;
			compressed_archive.cmp_len = compressBound(compressed_archive.src_len);

			// Allocate buffer to hold compressed data.
			compressed_archive.data.resize(compressed_archive.cmp_len);

			// Compress the data.
			s32 cmp_status = compress((u8*)compressed_archive.data.data(), &compressed_archive.cmp_len, (const u8*)a.data.data(), compressed_archive.src_len);
			if (cmp_status != Z_OK)
			{
				log::error("compress() failed!");
				return {};
			}

			log::info("Compressed from {0} to {1} bytes", compressed_archive.src_len, compressed_archive.cmp_len);

			return compressed_archive;
		}

		// ------------------------------------------------------------------------
		// Write the compressed archive to a file.
		bool write_archive(const std::string& outputPath, const resource_pipeline::CompressedArchive& archive)
		{
			std::ofstream ofs;
			ofs.open(outputPath, std::ios::out | std::ios::trunc | std::ios::binary);

			// Check if the file is successfully opened.
			if (ofs.is_open())
			{
				size_t sizeof_archive = archive.src_len * sizeof(u8);

				// Write the size of the uncompressed data followed by the compressed data.
				ofs.write(reinterpret_cast<const char8*>(&sizeof_archive), sizeof(size_t));
				ofs.write(reinterpret_cast<const char8*>(&archive.data[0]), archive.cmp_len * sizeof(u8));

				ofs.close();

				log::info("Written ({0} bytes) archive to disk {1}", (archive.cmp_len * sizeof(u8)) + sizeof(size_t), outputPath);

				return true;
			}

			log::error("Failed to write {0} to disk", outputPath);

			return false;
		}

		// ------------------------------------------------------------------------
		// Cook content by generating and compressing an archive.
		bool cook_content(const std::string& root, const std::vector<std::string>& subDirs)
		{
			// Generate an archive and compress it.
			resource_pipeline::CompressedArchive compressed_archive = compress_archive(archive_generator::generate(root, subDirs));

			// Write the compressed archive to a file.
			return write_archive(resource_pipeline::make_archive_path(root, subDirs), compressed_archive);
		}
	}
}