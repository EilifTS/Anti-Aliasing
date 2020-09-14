#include "camera.h"
#include "device.h"
#include "command_context.h"
#include "cpu_buffer.h"

egx::Camera::Camera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view)
	: window_size(window_size), near_plane(near_plane), far_plane(far_plane), field_of_view(field_of_view), 
	view_matrix(ema::mat4::Identity()), projection_matrix(ema::mat4::Identity()),
	buffer_updated(false), buffer(dev, (int)sizeof(CameraBufferType))
{
	updateProjectionMatrix();
}

void egx::Camera::UpdateBuffer(Device& dev, CommandContext& context)
{
	if (!buffer_updated)
	{
		CameraBufferType cbt;
		cbt.view_matrix = view_matrix.Transpose();
		cbt.projection_matrix = projection_matrix.Transpose();
		cbt.camera_position = ema::vec4(position, 1.0f);

		CPUBuffer cpu_buffer(&cbt, (int)sizeof(cbt));
		context.SetTransitionBuffer(buffer, GPUBufferState::CopyDest);
		dev.ScheduleUpload(context, cpu_buffer, buffer);
		context.SetTransitionBuffer(buffer, GPUBufferState::ConstantBuffer);
		buffer_updated = true;
	}
}

void egx::Camera::updateViewMatrix()
{
	view_matrix = ema::mat4::LookAt(position, look_at, up);
	buffer_updated = false;
}
void egx::Camera::updateProjectionMatrix()
{
	projection_matrix = ema::mat4::Projection(near_plane, far_plane, field_of_view, AspectRatio());
	buffer_updated = false;
}

egx::FPCamera::FPCamera(Device& dev, CommandContext& context, const ema::vec2& window_size, float near_plane, float far_plane, float field_of_view, float speed, float mouse_speed)
	: Camera(dev, context, window_size, near_plane, far_plane, field_of_view), speed(speed), mouse_speed(mouse_speed), right(), roll_pitch_yaw()
{
	look_at = ema::vec3(0.0f, 0.0f, 1.0f);
	up = ema::vec3(0.0f, 1.0f, 0.0f);
}

void egx::FPCamera::Update(const eio::InputManager& im)
{
	auto& keyboard = im.Keyboard();
	auto& mouse = im.Mouse();
	auto mouse_movement = mouse.Movement();

	float dt = (float)im.Clock().FrameTime();

	if (keyboard.IsKeyDown('W'))
		position += (look_at - position) * dt * speed;
	if (keyboard.IsKeyDown('S'))
		position -= (look_at - position) * dt * speed;
	if (keyboard.IsKeyDown('D'))
		position += right * dt * speed;
	if (keyboard.IsKeyDown('A'))
		position -= right * dt * speed;
	if (keyboard.IsKeyDown(32))
		position += up * dt * speed;
	if (keyboard.IsKeyDown(16))
		position -= up * dt * speed;

	roll_pitch_yaw += ema::vec3(0.0f, (float)mouse_movement.y * mouse_speed, (float)mouse_movement.x * mouse_speed);

	auto m_roll_pitch_yaw = ema::mat4::RollPitchYaw(roll_pitch_yaw);
	auto forward4 = ema::vec4(0.0f, 0.0f, 1.0f, 1.0f) * m_roll_pitch_yaw;
	auto right4 = ema::vec4(1.0f, 0.0f, 0.0f, 1.0f) * m_roll_pitch_yaw;
	auto up4 = ema::vec4(0.0f, 1.0f, 0.0f, 1.0f) * m_roll_pitch_yaw;

	look_at = position + ema::vec3(forward4.x, forward4.y, forward4.z);
	right = ema::vec3(right4.x, right4.y, right4.z);
	up = ema::vec3(up4.x, up4.y, up4.z);

	updateViewMatrix();
}