#pragma once
#include <cstdint>
#include <cmath>

namespace xs
{
	using uchar = unsigned char;
	struct color
	{
		union
		{
			uchar rgba[4];
			struct { uchar a, b, g, r; };
			uint32_t integer_value;
		};

		color operator +(const color& rhs)  const
		{
			return
			{
				uchar(a + rhs.a),
				uchar(b + rhs.b),
				uchar(g + rhs.g),
				uchar(r + rhs.r)
			};
		}

		color operator *(const color& rhs)  const
		{
			return
			{
				uchar(round(a * rhs.a / 255.0)),
				uchar(round(b * rhs.b / 255.0)),
				uchar(round(g * rhs.g / 255.0)),
				uchar(round(r * rhs.r / 255.0))
			};
		}
	};
}