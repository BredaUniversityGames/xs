#pragma once

#include <vector>

using s8 = signed char;
using u8 = unsigned char;
using s16 = signed short;
using u16 = unsigned short;
using s32 = signed int;
using u32 = unsigned int;
using s64 = signed long long;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

using slong = signed long;
using ulong = unsigned long;

// characters
using char8 = char;
using char16 = char16_t;
using char32 = char32_t;
using tchar = wchar_t;

namespace xs
{
	using Blob = std::vector<std::byte>;
}