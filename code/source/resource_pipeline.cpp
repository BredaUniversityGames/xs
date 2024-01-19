#include "resource_pipeline.h"

#include <sstream>

namespace xs::resource_pipeline
{
	namespace internal
	{
		// ------------------------------------------------------------------------
		std::string make_name(const std::vector<std::string>& subDirs)
		{
			// Names that should not be included into the archive name when exported
			static std::unordered_set<std::string> blacklist = { "shared" };

			std::stringstream stream;

			for (s32 i = 0; i < subDirs.size(); ++i)
			{
				if (blacklist.find(subDirs[i]) == blacklist.end())
				{
					stream << subDirs[i];

					// If we are NOT processing the last element then add a "_"
					if (i != subDirs.size() - 1)
					{
						stream << "_";
					}
				}
			}

			return stream.str();
		}
	}

	// ------------------------------------------------------------------------
	ContentHeader::ContentHeader()
		: file_path()
		, file_offset(0)
		, file_size(0)
		, file_size_compressed(0)
	{

	}

	// ------------------------------------------------------------------------
	ContentHeader::ContentHeader(const std::string& path, u64 fileOffset, u64 fileSize, u64 fileSizeCompressed)
		: file_path()
		, file_offset(fileOffset)
		, file_size(fileSize)
		, file_size_compressed(fileSizeCompressed)
	{
		size_t filePathLength = std::min(path.size(), static_cast<size_t>(s_max_path - 1));
		std::copy(path.begin(), path.begin() + filePathLength, file_path);
		file_path[filePathLength] = '\0'; // Null-terminate the file path

		for (int i = 0; file_path[i] != '\0'; i++) 
		{
			if (file_path[i] == '\\') 
				file_path[i] = '/';
		}
	}

	// ------------------------------------------------------------------------
	bool ContentHeader::operator==(const ContentHeader& other) const
	{
		return file_size == other.file_size && file_offset == other.file_offset && file_path == other.file_path && file_size_compressed == other.file_size_compressed;
	}

	// ------------------------------------------------------------------------
	bool ContentHeader::operator!=(const ContentHeader& other) const
	{
		return (*this) == other;
	}

	// ------------------------------------------------------------------------
	std::string make_archive_path(const std::string& root, const std::vector<std::string>& subDirs)
	{
		return root + "/" + internal::make_name(subDirs) + ".DATA";
	}

	// ------------------------------------------------------------------------
	const std::unordered_set<std::string>& supported_text_file_formats()
	{
		static std::unordered_set<std::string> file_formats =
		{
			".wren",		// scripting
			".frag",		// shaders
			".vert",		// shaders
			".json",		// text
		};

		return file_formats;
	}

	// ------------------------------------------------------------------------
	const std::unordered_set<std::string>& supported_binary_file_formats()
	{
		static std::unordered_set<std::string> file_formats =
		{
			".ttf",			// fonts
			".otf",			// fonts
			".png",			// images
			".bank",		// audio
			".wav"			// audio
		};

		return file_formats;
	}

	// ------------------------------------------------------------------------
	bool is_text_file(const std::string& extension)
	{
		return supported_text_file_formats().find(extension) != supported_text_file_formats().end();
	}

	// ------------------------------------------------------------------------
	bool is_binary_file(const std::string& extension)
	{
		return supported_binary_file_formats().find(extension) != supported_binary_file_formats().end();
	}

	// ------------------------------------------------------------------------
	bool is_supported_file_format(const std::string& extension)
	{
		return is_text_file(extension) || is_binary_file(extension);
	}
}