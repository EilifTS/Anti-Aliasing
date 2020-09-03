#pragma once
#include <cmath>
#include <DirectXPackedVector.h>
#include "vec2.h"


namespace ema
{
	class vec3
	{
	public:
		float x, y, z;

	public:
		inline vec3()
			: x(0.0f), y(0.0f), z(0.0f) {};
		explicit inline vec3(float v)
			: x(v), y(v), z(v) {};
		inline vec3(float x, float y, float z)
			: x(x), y(y), z(z) {};
		inline vec3(const vec2& xy, float z)
			: x(xy.x), y(xy.y), z(z) {};
		inline explicit vec3(const DirectX::XMFLOAT3& rhs)
			: x(rhs.x), y(rhs.y), z(rhs.z) {};

		inline float Dot(const vec3& rhs) const
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*) & rhs);
			d1 = DirectX::XMVector3Dot(d1, d2);
			return DirectX::XMVectorGetX(d1);
		};
		inline vec3 Cross(const vec3& rhs) const
		{
			return vec3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
		}

		inline float LengthSquared() const
		{
			return Dot(*this);
		};
		inline float Length() const
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
			d = DirectX::XMVector3Length(d);
			return DirectX::XMVectorGetX(d);
		};
		inline float LengthEst() const
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
			d = DirectX::XMVector3LengthEst(d);
			return DirectX::XMVectorGetX(d);
		};
		inline float Angle(const vec3& rhs) const
		{
			return std::acos(Dot(rhs) / (Length() * rhs.Length()));
		}

		inline bool IsZero() const
		{
			return x == 0.0f && y == 0.0f && z == 0.0f;
		}
		inline void Normalize()
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
			d = DirectX::XMVector3Normalize(d);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)this, d);
		}
		inline void NormalizeEst()
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
			d = DirectX::XMVector3NormalizeEst(d);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)this, d);
		}
		inline vec3 GetNormalized() const
		{
			vec3 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
			d = DirectX::XMVector3Normalize(d);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d);
			return out;
		};
		inline vec3 GetNormalizedEst() const
		{
			vec3 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
			d = DirectX::XMVector3NormalizeEst(d);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d);
			return out;
		};

		inline vec3& operator+=(const vec3& rhs)
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*) & rhs);
			d1 = DirectX::XMVectorAdd(d1, d2);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)this, d1);
			return *this;
		};
		inline vec3& operator-=(const vec3& rhs)
		{
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*) & rhs);
			d1 = DirectX::XMVectorSubtract(d1, d2);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)this, d1);
			return *this;
		};
		inline vec3 operator+(const vec3& rhs) const
		{
			vec3 out;
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*) & rhs);
			d1 = DirectX::XMVectorAdd(d1, d2);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d1);
			return out;
		};
		inline vec3 operator-(const vec3& rhs) const
		{
			vec3 out;
			DirectX::XMVECTOR d1 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			DirectX::XMVECTOR d2 = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*) & rhs);
			d1 = DirectX::XMVectorSubtract(d1, d2);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d1);
			return out;
		};
		inline vec3 operator-() const
		{
			vec3 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			d = DirectX::XMVectorNegate(d);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d);
			return out;
		};
		inline vec3& operator*=(float rhs)
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			d = DirectX::XMVectorScale(d, rhs);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)this, d);
			return *this;
		};
		inline vec3& operator/=(float rhs)
		{
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			d = DirectX::XMVectorScale(d, 1.0f / rhs);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)this, d);
			return *this;
		};
		inline vec3 operator*(float rhs) const
		{
			vec3 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			d = DirectX::XMVectorScale(d, rhs);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d);
			return out;
		};
		inline vec3 operator/(float rhs) const
		{
			vec3 out;
			DirectX::XMVECTOR d = DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)this);
			d = DirectX::XMVectorScale(d, 1.0f / rhs);
			DirectX::XMStoreFloat3((DirectX::XMFLOAT3*) & out, d);
			return out;
		};
		inline float& operator[](int index)
		{
			return ((float*)this)[index];
		}

		inline operator DirectX::XMFLOAT3() const
		{
			return DirectX::XMFLOAT3(x, y, z);
		}
	};

	inline vec3 operator*(float lhs, const vec3& a)
	{
		return a * lhs; // assuming commutativity
	};
}