#include "resource_pipeline.h"

#include <sstream>

#include "tools.h"

namespace xs::resource_pipeline
{

namespace internal
{
	std::string make_name(const std::vector<std::string>& sub_dirs)
	{
		// Names that should not be included into the archive name when exported
		static std::unordered_set<std::string> blacklist = { "shared" };

		std::stringstream stream;

		for (int i = 0; i < sub_dirs.size(); ++i)
		{
			if (blacklist.find(sub_dirs[i]) == blacklist.end())
			{
				stream << sub_dirs[i];

				// If we are NOT processing the last element then add a "_"
				if (i != sub_dirs.size() - 1)
				{
					stream << "_";
				}
			}
		}

		return stream.str();
	}
}

content_header::content_header()
	: file_path()
	, file_offset(0)
	, file_size(0)
	, file_size_compressed(0)
{}

content_header::content_header(
	const std::string& path,
	unsigned long long file_offset,
	unsigned long long file_size,
	unsigned long long file_size_compressed)
	: file_path()
	, file_offset(file_offset)
	, file_size(file_size)
	, file_size_compressed(file_size_compressed)
{
	auto rel_path = xs::tools::string_replace(path, "./games", "[games]");
	size_t filePathLength = std::min(rel_path.size(), static_cast<size_t>(s_max_path - 1));
	std::copy(rel_path.begin(), rel_path.begin() + filePathLength, file_path);
	file_path[filePathLength] = '\0'; // Null-terminate the file path

	for (int i = 0; file_path[i] != '\0'; i++) 
	{
		if (file_path[i] == '\\') 
			file_path[i] = '/';
	}
}

bool content_header::operator==(const content_header& other) const
{
	return	file_size == other.file_size &&
			file_offset == other.file_offset &&
			file_path == other.file_path &&
			file_size_compressed == other.file_size_compressed;
}

bool content_header::operator!=(const content_header& other) const
{
	return (*this) == other;
}

std::string make_archive_path(
	const std::string& root,
	const std::vector<std::string>& sub_dirs)
{
	return root + "/" + internal::make_name(sub_dirs) + ".xs";
}

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

bool is_text_file(const std::string& extension)
{
	return supported_text_file_formats().find(extension) != supported_text_file_formats().end();
}

bool is_binary_file(const std::string& extension)
{
	return supported_binary_file_formats().find(extension) != supported_binary_file_formats().end();
}

bool is_supported_file_format(const std::string& extension)
{
	return is_text_file(extension) || is_binary_file(extension);
}

}