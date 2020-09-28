#include "model.h"
#include "cpu_buffer.h"
#include "device.h"
#include "command_context.h"

namespace
{
	struct ModelBufferType
	{
		ema::mat4 world_matrix;
		ema::mat4 last_world_matrix;
	};
}

egx::Model::Model(Device& dev, const std::vector<std::shared_ptr<Mesh>>& meshes)
	: meshes(meshes),
	model_buffer(dev, (int)sizeof(ModelBufferType)),
	is_static(false),
	position(0.0f),
	rotation(0.0f),
	scale(1.0f)
{
}
void egx::Model::UpdateBuffer(Device& dev, CommandContext& context)
{
	ModelBufferType mbt;
	mbt.world_matrix = CalculateWorldMatrix();
	mbt.world_matrix = mbt.world_matrix.Transpose();
	mbt.last_world_matrix = last_world_matrix;
	last_world_matrix = mbt.world_matrix;

	CPUBuffer cpu_buffer(&mbt, (int)sizeof(mbt));
	context.SetTransitionBuffer(model_buffer, GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, model_buffer);
	context.SetTransitionBuffer(model_buffer, GPUBufferState::ConstantBuffer);

}

ema::mat4 egx::Model::CalculateWorldMatrix()
{
	return ema::mat4::Scale(scale) * ema::mat4::RollPitchYaw(rotation) * ema::mat4::Translation(position);
}