#pragma once
#pragma once
#include "vec4.h"

namespace ema
{
	class color
	{
	public:
		union
		{
			ema::vec4 vec;
			struct {
				float r;
				float g;
				float b;
				float a;
			};
		};

	public:
		color()
			: vec()
		{

		}
		color(float r, float g, float b, float a)
			: vec(r, g, b, a)
		{

		}
		explicit color(const ema::vec4& vec)
			: vec(vec)
		{

		}

		static color FromSRGB(const color& c)
		{
			const float gamma = 2.2f;
			return color(c.vec.Power(gamma));
		}

		inline color& operator+=(const color& rhs)
		{
			this->vec += rhs.vec;
			return *this;
		};
		inline color& operator-=(const color& rhs)
		{
			this->vec -= rhs.vec;
			return *this;
		};
		inline color operator+(const color& rhs) const
		{
			return color(this->vec + rhs.vec);
		};
		inline color operator-(const color& rhs) const
		{
			return color(this->vec - rhs.vec);
		};
		inline color operator-() const
		{
			return color(-this->vec);
		};
		inline color& operator*=(float rhs)
		{
			this->vec *= rhs;
			return *this;
		};
		inline color& operator/=(float rhs)
		{
			this->vec /= rhs;
			return *this;
		};
		inline color operator*(float rhs) const
		{
			return color(this->vec * rhs);
		};
		inline color operator/(float rhs) const
		{
			return color(this->vec / rhs);
		};


	};
	inline color operator*(float lhs, const color& a)
	{
		return a * lhs; // assuming commutativity
	};
}