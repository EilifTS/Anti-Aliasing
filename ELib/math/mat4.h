#pragma once
#include <cmath>
#include <DirectXMath.h>
#include "vec4.h"

namespace ema
{
	class mat4
	{
	private:
		DirectX::XMMATRIX matrix;

	public:
		static inline mat4 Identity()
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixIdentity();
			return out;
		}
		static inline mat4 Translation(vec3 offset)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixTranslation(offset.x, offset.y, offset.z);
			return out;
		}

		inline float Determinant() const
		{
			auto determinant = DirectX::XMMatrixDeterminant(matrix);
			return DirectX::XMVectorGetX(determinant);
		}
		inline mat4 Inverse() const
		{
			mat4 out;
			auto determinant = DirectX::XMMatrixDeterminant(matrix);
			out.matrix = DirectX::XMMatrixInverse(&determinant, matrix);
			return out;
		}
		inline mat4 Transpose() const
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixTranspose(matrix);
			return out;
		}
		inline const DirectX::XMMATRIX& GetDXMatrix() const { return matrix; };
		
		inline mat4 operator*(const mat4& rhs) const
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixMultiply(matrix, rhs.matrix);
			return out;
		};
		inline mat4& operator*=(const mat4& rhs)
		{
			matrix = DirectX::XMMatrixMultiply(matrix, rhs.matrix);
			return *this;
		};

		inline vec4 operator[](int index) const
		{
			auto row_vector = matrix.r[index];
			DirectX::XMFLOAT4 row_floats;
			DirectX::XMStoreFloat4(&row_floats, row_vector);
			return vec4(row_floats);
		}

	};

	
}

inline ema::vec4 operator*(const ema::vec4& lhs, const ema::mat4& rhs)
{
	DirectX::XMVECTOR d = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & lhs);
	auto out = DirectX::XMVector4Transform(d, rhs.GetDXMatrix());
	ema::vec4 outv;
	DirectX::XMStoreFloat4A((DirectX::XMFLOAT4A*) & outv, out);
	return outv;
};