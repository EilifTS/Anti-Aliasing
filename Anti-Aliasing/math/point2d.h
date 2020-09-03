#pragma once

namespace ema
{
	class point2D
	{
	public:
		int x;
		int y;

	public:
		inline point2D()
			: x(0), y(0)
		{}
		explicit inline point2D(int v)
			: x(v), y(v)
		{}
		inline point2D(int x, int y)
			: x(x), y(y)
		{}

		inline point2D& operator+=(const point2D& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		inline point2D& operator-=(const point2D& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		inline point2D operator+(const point2D& rhs)const
		{
			return point2D(x + rhs.x, y + rhs.y);
		}
		inline point2D operator-(const point2D& rhs)const
		{
			return point2D(x - rhs.x, y - rhs.y);
		}
		inline point2D operator*(int rhs) const
		{
			return point2D(x * rhs, y * rhs);
		}
		inline point2D operator*=(int rhs)
		{
			auto a = *this * rhs;
			*this = a;
			return *this;
		}
		inline point2D operator/(int rhs) const
		{
			return point2D(x / rhs, y / rhs);
		}
		inline point2D operator/=(int rhs)
		{
			auto a = *this / rhs;
			*this = a;
			return *this;
		}

		inline point2D operator*(const point2D& rhs) const
		{
			return point2D(x * rhs.x, y * rhs.y);
		}
		inline point2D operator*=(const point2D& rhs)
		{
			auto a = *this * rhs;
			*this = a;
			return *this;
		}

		inline bool operator==(const point2D& rhs)
		{
			return x == rhs.x && y == rhs.y;
		}
		inline bool operator!=(const point2D& rhs)
		{
			return !(*this == rhs);
		}
	};

	inline point2D operator*(int lhs, const point2D& rhs)
	{
		return rhs * lhs; // assuming commutativity
	};
}


