#include "camera.h"
#include "device.h"
#include "command_context.h"
#include "cpu_buffer.h"

namespace
{
	struct cameraBufferType
	{
		ema::mat4 view_matrix;
		ema::mat4 inv_view_matrix;
		ema::mat4 projection_matrix;
		ema::mat4 inv_projection_matrix;
		ema::mat4 projection_matrix_no_jitter;
		ema::mat4 inv_projection_matrix_no_jitter;
	};
}

// Camera
egx::Camera::Camera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane)
	: window_size(window_size), near_plane(near_plane), far_plane(far_plane), 
	view_matrix(ema::mat4::Identity()), projection_matrix(ema::mat4::Identity())
{
	curr_buffer = std::make_shared<egx::ConstantBuffer>(dev, (int)sizeof(cameraBufferType));
	last_buffer = std::make_shared<egx::ConstantBuffer>(dev, (int)sizeof(cameraBufferType));
}

void egx::Camera::UpdateBuffer(Device& dev, CommandContext& context)
{
	// Swap buffers
	cameraBufferType cbt;
	cbt.view_matrix = view_matrix.Transpose();
	cbt.inv_view_matrix = inv_view_matrix.Transpose();
	cbt.projection_matrix = projection_matrix.Transpose();
	cbt.inv_projection_matrix = inv_projection_matrix.Transpose();
	cbt.projection_matrix_no_jitter = projection_matrix_no_jitter.Transpose();
	cbt.inv_projection_matrix_no_jitter = inv_projection_matrix_no_jitter.Transpose();


	context.SetTransitionBuffer(*curr_buffer, GPUBufferState::CopySource);
	context.SetTransitionBuffer(*last_buffer, GPUBufferState::CopyDest);
	context.CopyBuffer(*curr_buffer, *last_buffer);

	CPUBuffer cpu_buffer(&cbt, (int)sizeof(cbt));
	context.SetTransitionBuffer(*curr_buffer, GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, *curr_buffer);
	context.SetTransitionBuffer(*curr_buffer, GPUBufferState::ConstantBuffer);
	context.SetTransitionBuffer(*last_buffer, GPUBufferState::ConstantBuffer);
}

void egx::Camera::UpdateViewMatrix()
{
	view_matrix = ema::mat4::LookAt(position, look_at, up);
	inv_view_matrix = view_matrix.Inverse();
}

// Projective Camera
egx::ProjectiveCamera::ProjectiveCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view)
	: Camera(dev, context, window_size, near_plane, far_plane), field_of_view(field_of_view)
{
	updateProjectionMatrixNoJitter();
}
void egx::ProjectiveCamera::updateProjectionMatrixNoJitter()
{
	near_plane_vs_rectangle = ema::vec2(AspectRatio() * tanf(0.5f * field_of_view), tanf(0.5f * field_of_view));

	projection_matrix_no_jitter = ema::mat4::Projection(near_plane, far_plane, near_plane_vs_rectangle * near_plane);
	inv_projection_matrix_no_jitter = ema::mat4::ProjectionOffsetInverse(near_plane, far_plane, near_plane_vs_rectangle * near_plane, ema::vec2());
}
void egx::ProjectiveCamera::updateProjectionMatrix()
{
	near_plane_vs_rectangle = ema::vec2(AspectRatio() * tanf(0.5f * field_of_view), tanf(0.5f * field_of_view));

	//ema::vec2 final_jitter = ema::vec2(jitter.x, jitter.y) / window_size.x * 2.0f;
	//ema::vec2 final_jitter = ema::vec2((jitter.x - 0.5f) / window_size.x, (jitter.y - 0.5f) / window_size.y) * 2.0f;
	ema::vec2 final_jitter = ema::vec2((jitter.x - 0.5f) / window_size.x, -(jitter.y - 0.5f) / window_size.y) * 2.0f;
	ema::vec2 final_jitter2 = ema::vec2((jitter.x - 0.5f), -(jitter.y - 0.5f)) / window_size.x * 2.0f;
	projection_matrix = ema::mat4::ProjectionOffset(near_plane, far_plane, near_plane_vs_rectangle * near_plane, final_jitter);
	inv_projection_matrix = ema::mat4::ProjectionOffsetInverse(near_plane, far_plane, near_plane_vs_rectangle * near_plane, final_jitter);

	// Jittering test
	ema::vec4 pixel_pos = ema::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	ema::vec4 view_pixel_pos = pixel_pos * inv_projection_matrix_no_jitter;
	ema::vec4 j_pixel_pos = view_pixel_pos * projection_matrix;
	ema::vec2 temp = ema::vec2(j_pixel_pos.x, j_pixel_pos.y) * 0.5f + ema::vec2(0.5f, 0.5f);
	ema::vec2 f_pos = ema::vec2(temp.x * window_size.x, temp.y * window_size.y);
	

	  // Inverse check
	auto id = projection_matrix * inv_projection_matrix;
	auto inv = projection_matrix.Inverse();
	
}

// First person camera
egx::FPCamera::FPCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view, float speed, float mouse_speed)
	: ProjectiveCamera(dev, context, window_size, near_plane, far_plane, field_of_view), speed(speed), mouse_speed(mouse_speed), right(), roll_pitch_yaw()
{
	look_at = ema::vec3(0.0f, 0.0f, 1.0f);
	up = ema::vec3(0.0f, 1.0f, 0.0f);
}


void egx::FPCamera::HandleInput(const eio::InputManager& im)
{
	auto& keyboard = im.Keyboard();
	auto& mouse = im.Mouse();
	auto mouse_movement = mouse.Movement();

	float dt = (float)im.Clock().FrameTime();
	float c_speed = speed;

	if (keyboard.IsKeyDown(17))
		c_speed *= 100.0f;

	if (keyboard.IsKeyDown('W'))
		position += (look_at - position) * dt * c_speed;
	if (keyboard.IsKeyDown('S'))
		position -= (look_at - position) * dt * c_speed;
	if (keyboard.IsKeyDown('D'))
		position += right * dt * c_speed;
	if (keyboard.IsKeyDown('A'))
		position -= right * dt * c_speed;
	if (keyboard.IsKeyDown(32))
		position += ema::vec3(0.0f, 1.0f, 0.0f) * dt * c_speed;
	if (keyboard.IsKeyDown(16))
		position -= ema::vec3(0.0f, 1.0f, 0.0f) * dt * c_speed;

	roll_pitch_yaw += ema::vec3(0.0f, (float)mouse_movement.y * mouse_speed, (float)mouse_movement.x * mouse_speed);

	auto m_roll_pitch_yaw = ema::mat4::RollPitchYaw(roll_pitch_yaw);
	auto forward4 = ema::vec4(0.0f, 0.0f, 1.0f, 1.0f) * m_roll_pitch_yaw;
	auto right4 = ema::vec4(1.0f, 0.0f, 0.0f, 1.0f) * m_roll_pitch_yaw;
	auto up4 = ema::vec4(0.0f, 1.0f, 0.0f, 1.0f) * m_roll_pitch_yaw;

	look_at = position + ema::vec3(forward4.x, forward4.y, forward4.z);
	right = ema::vec3(right4.x, right4.y, right4.z);
	up = ema::vec3(up4.x, up4.y, up4.z);
}

void egx::FPCamera::Update()
{
	updateProjectionMatrix();

	last_view_matrix = view_matrix;
	view_matrix = ema::mat4::Translation(-position) * ema::mat4::LookAt(ema::vec3(), look_at - position, up);	
	inv_view_matrix = ema::mat4::LookAt(ema::vec3(), look_at - position, up).Transpose() * ema::mat4::Translation(position);
}

// Orthographic camera
egx::OrthographicCamera::OrthographicCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane)
	: Camera(dev, context, window_size, near_plane, far_plane)
{
	updateProjectionMatrix();
}

void egx::OrthographicCamera::updateProjectionMatrix()
{
	projection_matrix = ema::mat4::Orthographic(window_size, near_plane, far_plane);
	projection_matrix_no_jitter = projection_matrix;
	inv_projection_matrix = projection_matrix.Inverse();
	near_plane_vs_rectangle = ema::vec2(AspectRatio(), 1.0f); // Not right but not really needed
}