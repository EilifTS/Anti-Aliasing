#pragma once
#include "../math/mat4.h"
#include "constant_buffer.h"
#include "../io/input_manager.h"

namespace egx
{
	

	class Camera
	{
	public:
		Camera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane);


		void UpdateViewMatrix();
		void UpdateBuffer(Device& dev, CommandContext& context);

		inline egx::ConstantBuffer& GetLastBuffer() { return *last_buffer; };
		inline egx::ConstantBuffer& GetBuffer() { return *curr_buffer; };
		inline float FarPlane() const { return far_plane; };
		inline float NearPlane() const { return near_plane; };
		inline float AspectRatio() const { return window_size.x / window_size.y; };
		inline const ema::vec3& Position() const { return position; };
		inline const ema::vec3& LookAt() const { return look_at; };

		inline const ema::mat4& ViewMatrix() const{ return view_matrix; };
		inline const ema::mat4& InvViewMatrix() const{ return inv_view_matrix; };
		inline const ema::mat4& ProjectionMatrix() const{ return projection_matrix; };
		inline const ema::mat4& InvProjectionMatrix() const{ return inv_projection_matrix; };
		inline const ema::mat4& ProjectionMatrixNoJitter() const{ return projection_matrix_no_jitter; };
		inline const ema::mat4& InvProjectionMatrixNoJitter() const{ return inv_projection_matrix_no_jitter; };
		inline const ema::mat4& LastProjectionMatrixNoJitter() const{ return projection_matrix_no_jitter; }; // Does not change
		inline const ema::mat4& LastViewMatrix() const{ return last_view_matrix; };

		inline void SetPosition(const ema::vec3& new_pos) { position = new_pos; };
		inline void SetLookAt(const ema::vec3& new_look_at) { look_at = new_look_at; };
		inline void SetUp(const ema::vec3& new_up) { up = new_up; };
		inline void SetJitter(const ema::vec2& new_jitter) { jitter = new_jitter; };

	protected:

	protected:
		ema::vec2 window_size;
		ema::vec2 near_plane_vs_rectangle;
		float near_plane, far_plane;

		ema::vec3 position;
		ema::vec3 look_at;
		ema::vec3 up;

		ema::mat4 view_matrix;
		ema::mat4 inv_view_matrix;
		ema::mat4 projection_matrix;
		ema::mat4 inv_projection_matrix;
		ema::mat4 projection_matrix_no_jitter;
		ema::mat4 inv_projection_matrix_no_jitter;
		ema::mat4 last_view_matrix;

		ema::vec2 jitter;

		std::shared_ptr<egx::ConstantBuffer> last_buffer;
		std::shared_ptr<egx::ConstantBuffer> curr_buffer;
	};

	class ProjectiveCamera : public Camera
	{
	public:
		ProjectiveCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view);

	protected:
		void updateProjectionMatrixNoJitter();
		void updateProjectionMatrix();

	protected:
		float field_of_view;

	};

	class FPCamera : public ProjectiveCamera
	{
	public:
		FPCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view, float speed, float mouse_speed);

		void HandleInput(const eio::InputManager& im);

		void Update();

		void SetRotation(const ema::vec3& new_roll_pitch_yaw);
		inline const ema::vec3& GetRotation() const { return roll_pitch_yaw; };

	private:
		float speed, mouse_speed;
		ema::vec3 right;
		ema::vec3 roll_pitch_yaw;
		bool roll_pitch_yaw_changed = true;
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