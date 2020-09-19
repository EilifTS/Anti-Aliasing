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
		static inline mat4 Translation(const vec3& offset)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixTranslation(offset.x, offset.y, offset.z);
			return out;
		}
		static inline mat4 Scale(const vec3& scale)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
			return out;
		}
		static inline mat4 Projection(float near_plane, float far_plane, float field_of_view, float aspect_ratio)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixPerspectiveFovLH(field_of_view, aspect_ratio, near_plane, far_plane);
			return out;
		}
		static inline mat4 Orthographic(const ema::vec2& dims, float near_plane, float far_plane)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixOrthographicLH(dims.x, dims.y, near_plane, far_plane);
			return out;
		}
		static inline mat4 LookAt(const vec3& position, const vec3& look_at, const vec3& up)
		{
			mat4 out;
			auto position_vector = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & position);
			auto look_at_vector = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & look_at);
			auto up_vector = DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & up);
			
			out.matrix = DirectX::XMMatrixLookAtLH(position_vector, look_at_vector, up_vector);
			return out;
		}
		static inline mat4 RollPitchYaw(const ema::vec3& roll_pitch_yaw)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixRotationRollPitchYaw(roll_pitch_yaw.y, roll_pitch_yaw.z, roll_pitch_yaw.x);
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
	DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & outv, out);
	return outv;
};