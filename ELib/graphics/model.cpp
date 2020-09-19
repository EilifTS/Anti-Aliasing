#include "model.h"
#include "cpu_buffer.h"
#include "device.h"
#include "command_context.h"

namespace
{
	struct ModelBufferType
	{
		ema::mat4 world_matrix;
		unsigned int is_static;
	};
}

egx::Model::Model(Device& dev, const std::vector<std::shared_ptr<Mesh>>& meshes)
	: meshes(meshes),
	model_buffer(dev, (int)sizeof(ModelBufferType)),
	is_static(false),
	outdated_constant_buffer(true),
	position(0.0f),
	rotation(0.0f),
	scale(1.0f)
{
}
void egx::Model::UpdateBuffer(Device& dev, CommandContext& context)
{
	if (outdated_constant_buffer)
	{
		ModelBufferType mbt;
		mbt.world_matrix = ema::mat4::Scale(scale) * ema::mat4::RollPitchYaw(rotation) * ema::mat4::Translation(position);
		mbt.world_matrix = mbt.world_matrix.Transpose();
		mbt.is_static = is_static ? 0 : 1;
		CPUBuffer cpu_buffer(&mbt, (int)sizeof(mbt));
		context.SetTransitionBuffer(model_buffer, GPUBufferState::CopyDest);
		dev.ScheduleUpload(context, cpu_buffer, model_buffer);
		context.SetTransitionBuffer(model_buffer, GPUBufferState::ConstantBuffer);

		outdated_constant_buffer = false;
	}
}