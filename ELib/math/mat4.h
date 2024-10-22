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
		static inline mat4 Projection(float near_plane, float far_plane, const ema::vec2& dims)
		{
			mat4 out;
			out.matrix = DirectX::XMMatrixPerspectiveLH(dims.x * 2.0f, dims.y * 2.0f, near_plane, far_plane);
			return out;
		}
		static inline mat4 ProjectionOffset(float near_plane, float far_plane, const ema::vec2& dims, const ema::vec2 offset)
		{
			mat4 out;

			float n = near_plane;
			float rw = 1.0f / dims.x;
			float rh = 1.0f / dims.y;
			float R = far_plane / (far_plane - near_plane);
			float a = offset.x;
			float b = offset.y;

			out.matrix.r[0] = DirectX::XMVectorSet(n * rw, 0.0f, 0.0f, 0.0f);
			out.matrix.r[1] = DirectX::XMVectorSet(0.0f, n * rh, 0.0f, 0.0f);
			out.matrix.r[2] = DirectX::XMVectorSet(-a, -b, R, 1.0f);
			out.matrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, -R*n, 0.0f);

			return out;
		}
		static inline mat4 ProjectionOffsetInverse(float near_plane, float far_plane, const ema::vec2& dims, const ema::vec2 offset)
		{
			mat4 out;
			float rec_n = 1.0f / near_plane;
			float rec_R = rec_n - 1.0f / far_plane;
			float w = dims.x;
			float h = dims.y;
			float a = offset.x;
			float b = offset.y;

			out.matrix.r[0] = DirectX::XMVectorSet(rec_n * w,	0.0f,		0.0f, 0.0f);
			out.matrix.r[1] = DirectX::XMVectorSet(0.0f,		rec_n * h,	0.0f, 0.0f);
			out.matrix.r[2] = DirectX::XMVectorSet(0.0f,		0.0f,		0.0f, -rec_R);
			out.matrix.r[3] = DirectX::XMVectorSet(a * rec_n * w,	b * rec_n * h,	1.0f, rec_n);

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