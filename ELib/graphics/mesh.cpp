#include "mesh.h"
#include "device.h"
#include "command_context.h"
#include "cpu_buffer.h"

egx::Mesh::Mesh(
	Device& dev,
	CommandContext& context,
	const std::string& name,
	const std::vector<MeshVertex>& vertices,
	const std::vector<unsigned long>& indices
)
	: name(name), 
	vertex_buffer(dev, (int)sizeof(MeshVertex), (int)vertices.size()),
	index_buffer(dev, (int)indices.size())
{
	CPUBuffer cpu_vertex_buffer(vertices.data(), (int)vertices.size() * (int)sizeof(MeshVertex));
	dev.ScheduleUpload(context, cpu_vertex_buffer, vertex_buffer);
	context.SetTransitionBuffer(vertex_buffer, GPUBufferState::VertexBuffer);

	CPUBuffer cpu_index_buffer(indices.data(), (int)indices.size() * (int)sizeof(unsigned long));
	dev.ScheduleUpload(context, cpu_index_buffer, index_buffer);
	context.SetTransitionBuffer(index_buffer, GPUBufferState::IndexBuffer);
}