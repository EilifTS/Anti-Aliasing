#pragma once
#include "../math/mat4.h"
#include "constant_buffer.h"
#include "../io/input_manager.h"

namespace egx
{
	struct CameraBufferType
	{
		ema::mat4 view_matrix;
		ema::mat4 projection_matrix;
		ema::vec2 near_plane_view_space_rectangle;
	};

	class Camera
	{
	public:
		Camera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane);


		void UpdateViewMatrix();
		void UpdateBuffer(Device& dev, CommandContext& context);

		inline egx::ConstantBuffer& GetBuffer() { return buffer; };
		inline float FarPlane() const { return far_plane; };
		inline float NearPlane() const { return near_plane; };
		inline float AspectRatio() const { return window_size.x / window_size.y; };
		inline const ema::vec3& Position() const { return position; };
		inline const ema::vec3& LookAt() const { return look_at; };
		inline const ema::mat4& ViewMatrix() const{ return view_matrix; };
		inline const ema::mat4& ProjectionMatrix() const{ return projection_matrix; };

		inline void SetPosition(const ema::vec3& new_pos) { position = new_pos; };
		inline void SetLookAt(const ema::vec3& new_look_at) { look_at = new_look_at; };
		inline void SetUp(const ema::vec3& new_up) { up = new_up; };

	protected:
		virtual void updateProjectionMatrix() = 0;

	protected:
		ema::vec2 window_size;
		ema::vec2 near_plane_vs_rectangle;
		float near_plane, far_plane;

		ema::vec3 position;
		ema::vec3 look_at;
		ema::vec3 up;

		ema::mat4 view_matrix;
		ema::mat4 projection_matrix;

		bool buffer_updated;
		egx::ConstantBuffer buffer;
	};

	class ProjectiveCamera : public Camera
	{
	public:
		ProjectiveCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view);

	protected:
		void updateProjectionMatrix();

	protected:
		float field_of_view;

	};

	class FPCamera : public ProjectiveCamera
	{
	public:
		FPCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view, float speed, float mouse_speed);

		void Update(const eio::InputManager& im);

		inline void SetRotation(const ema::vec3& new_roll_pitch_yaw) { roll_pitch_yaw = new_roll_pitch_yaw; };

	private:
		float speed, mouse_speed;
		ema::vec3 right;
		ema::vec3 roll_pitch_yaw;
	};

	class OrthographicCamera : public Camera
	{
	public:
		OrthographicCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane);

	protected:
		void updateProjectionMatrix();

	protected:

	};
}