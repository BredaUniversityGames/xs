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
			color c;
			c.a = uchar(a + rhs.a);
			c.b = uchar(b + rhs.b);
			c.g = uchar(g + rhs.g);
			c.r = uchar(r + rhs.r);
			return c;			
		}

		color operator *(const color& rhs)  const
		{
			color c;
			c.a = uchar(a * rhs.a / 255.0);
			c.b = uchar(b * rhs.b / 255.0);
			c.g = uchar(g * rhs.g / 255.0);
			c.r = uchar(r * rhs.r / 255.0);
			return c;
		}
	};
}