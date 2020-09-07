#pragma once
#include <cmath>
#include <DirectXMath.h>
#include "vec3.h"

namespace ema
{
	class vec4
	{
	public:
		float x, y, z, w;

	public:
		inline vec4()
			: x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
		explicit inline vec4(float v)
			: x(v), y(v), z(v), w(v) {};
		inline vec4(float x, float y, float z, float w)
			: x(x), y(y), z(z), w(w) {};
		inline vec4(const vec2& xy, const vec2& zw)
			: x(xy.x), y(xy.y), z(zw.x), w(zw.y) {};
		inline vec4(const vec3& xyz, float w)
			: x(xyz.x), y(xyz.y), z(xyz.z), w(w) {};
		inline explicit vec4(const DirectX::XMFLOAT4& rhs)
			: x(rhs.x), y(rhs.y), z(rhs.z), w(rhs.w) {};

		inline float Dot(const vec4& rhs) const
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & rhs);
			d1 = DirectX::XMVector4Dot(d1, d2);
			return DirectX::XMVectorGetX(d1);
		};

		inline float LengthSquared() const
		{
			return Dot(*this);
		};
		inline float Length() const
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			d = DirectX::XMVector4Length(d);
			return DirectX::XMVectorGetX(d);
		};
		inline float LengthEst() const
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			d = DirectX::XMVector4LengthEst(d);
			return DirectX::XMVectorGetX(d);
		};
		inline float Angle(const vec4& rhs) const
		{
			return std::acos(Dot(rhs) / (Length() * rhs.Length()));
		}
		inline vec4 Power(float power) const
		{
			vec4 out;
			DirectX::XMFLOAT4 p(power, power, power, power);
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat4(&p);
			d = DirectX::XMVectorPow(d, d2);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d);
			return out;
		};

		inline bool IsZero() const
		{
			return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f;
		}
		inline void Normalize()
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			d = DirectX::XMVector4Normalize(d);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)this, d);
		};
		inline void NormalizeEst()
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			d = DirectX::XMVector4NormalizeEst(d);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)this, d);
		};
		inline vec4 GetNormalized() const
		{
			vec4 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			d = DirectX::XMVector4Normalize(d);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d);
			return out;
		};
		inline vec4 GetNormalizedEst() const
		{
			vec4 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)this);
			d = DirectX::XMVector4NormalizeEst(d);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d);
			return out;
		};

		inline vec4& operator+=(const vec4& rhs)
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & rhs);
			d1 = DirectX::XMVectorAdd(d1, d2);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)this, d1);
			return *this;
		};
		inline vec4& operator-=(const vec4& rhs)
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & rhs);
			d1 = DirectX::XMVectorSubtract(d1, d2);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)this, d1);
			return *this;
		};
		inline vec4 operator+(const vec4& rhs) const
		{
			vec4 out;
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & rhs);
			d1 = DirectX::XMVectorAdd(d1, d2);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d1);
			return out;
		};
		inline vec4 operator-(const vec4& rhs) const
		{
			vec4 out;
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & rhs);
			d1 = DirectX::XMVectorSubtract(d1, d2);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d1);
			return out;
		};
		inline vec4 operator-() const
		{
			vec4 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			d = DirectX::XMVectorNegate(d);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d);
			return out;
		};
		inline vec4& operator*=(float rhs)
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			d = DirectX::XMVectorScale(d, rhs);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)this, d);
			return *this;
		};
		inline vec4& operator/=(float rhs)
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			d = DirectX::XMVectorScale(d, 1.0f / rhs);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)this, d);
			return *this;
		};
		inline vec4 operator*(float rhs) const
		{
			vec4 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			d = DirectX::XMVectorScale(d, rhs);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d);
			return out;
		};
		inline vec4 operator/(float rhs) const
		{
			vec4 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)this);
			d = DirectX::XMVectorScale(d, 1.0f / rhs);
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & out, d);
			return out;
		};
		inline float& operator[](int index)
		{
			return ((float*)this)[index];
		}

		inline operator DirectX::XMFLOAT4() const
		{
			return DirectX::XMFLOAT4(x, y, z, w);
		}
	};

	inline vec4 operator*(float lhs, const vec4& a)
	{
		return a * lhs; // assuming commutativity
	};
}