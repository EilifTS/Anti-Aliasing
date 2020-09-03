#pragma once
#include <cmath>
#include <DirectXMath.h>
#include "point2d.h"

namespace ema
{
	class vec2
	{
	public:
		float x, y;

	public:
		inline vec2()
			: x(0.0f), y(0.0f) {};
		explicit inline vec2(float v)
			: x(v), y(v) {};
		inline vec2(float x, float y)
			: x(x), y(y) {};
		inline explicit vec2(const point2D& rhs)
			: x((float)rhs.x), y((float)rhs.y) {};
		inline explicit vec2(const DirectX::XMFLOAT2& rhs)
			: x(rhs.x), y(rhs.y) {};

		inline float Dot(const vec2& rhs) const
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & rhs);
			d1 = DirectX::XMVector2Dot(d1, d2);
			return DirectX::XMVectorGetX(d1);
		};
		inline float Cross(const vec2& rhs) const
		{
			return x * rhs.y - y * rhs.x;
		}

		inline float LengthSquared() const
		{
			return Dot(*this);
		};
		inline float Length() const
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)this);
			d = DirectX::XMVector2Length(d);
			return DirectX::XMVectorGetX(d);
		};
		inline float LengthEst() const
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)this);
			d = DirectX::XMVector2LengthEst(d);
			return DirectX::XMVectorGetX(d);
		};
		inline float Angle(const vec2& rhs) const
		{
			return std::acos(Dot(rhs) / (Length() * rhs.Length()));
		}

		inline bool IsZero() const
		{
			return x == 0.0f && y == 0.0f;
		}
		inline void Normalize()
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)this);
			d = DirectX::XMVector2Normalize(d);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*)this, d);
		}
		inline void NormalizeEst()
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)this);
			d = DirectX::XMVector2NormalizeEst(d);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*)this, d);
		}
		inline vec2 GetNormalized() const
		{
			vec2 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)this);
			d = DirectX::XMVector2Normalize(d);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*) & out, d);
			return out;
		}
		inline vec2 GetNormalizedEst() const
		{
			vec2 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)this);
			d = DirectX::XMVector2NormalizeEst(d);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*) & out, d);
			return out;
		}
		inline void Rotate(float angle)
		{
			float c = cosf(angle);
			float s = sinf(angle);
			float ox = x;
			float oy = y;
			x = c * ox - s * oy;
			y = s * ox + c * oy;
		}

		inline vec2& operator+=(const vec2& rhs)
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & rhs);
			d1 = DirectX::XMVectorAdd(d1, d2);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*)this, d1);
			return *this;
		};
		inline vec2& operator-=(const vec2& rhs)
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & rhs);
			d1 = DirectX::XMVectorSubtract(d1, d2);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*)this, d1);
			return *this;
		};
		inline vec2 operator+(const vec2& rhs) const
		{
			vec2 out;
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & rhs);
			d1 = DirectX::XMVectorAdd(d1, d2);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*) & out, d1);
			return out;
		};
		inline vec2 operator-(const vec2& rhs) const
		{
			vec2 temp(*this);
			temp -= rhs;
			return temp;
		};
		inline vec2 operator-() const
		{
			vec2 temp;
			temp -= *this;
			return temp;
		};
		inline vec2& operator*=(float rhs)
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			d = DirectX::XMVectorScale(d, rhs);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*)this, d);
			return *this;
		};
		inline vec2& operator/=(float rhs)
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			d = DirectX::XMVectorScale(d, 1.0f / rhs);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*)this, d);
			return *this;
		};
		inline vec2 operator*(float rhs) const
		{
			vec2 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			d = DirectX::XMVectorScale(d, rhs);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*) & out, d);
			return out;
		};
		inline vec2 operator/(float rhs) const
		{
			vec2 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*)this);
			d = DirectX::XMVectorScale(d, 1.0f / rhs);
			DirectX::XMStoreFloat2((DirectX::XMFLOAT2*) & out, d);
			return out;
		};
		inline float& operator[](int index)
		{
			return ((float*)this)[index];
		}

		explicit inline operator point2D() const
		{
			return point2D((int)x, (int)y);
		}
		explicit inline operator DirectX::XMFLOAT2() const
		{
			return DirectX::XMFLOAT2(x, y);
		}
	};

	inline vec2 operator*(float lhs, const vec2& a)
	{
		return a * lhs;
	};
}