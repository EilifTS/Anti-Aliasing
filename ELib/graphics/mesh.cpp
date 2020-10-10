#include "mesh.h"
#include "device.h"
#include "command_context.h"
#include "cpu_buffer.h"
#include "../io/mesh_io.h"

egx::Mesh::Mesh(
	Device& dev,
	CommandContext& context,
	const std::string& name,
	const std::vector<MeshVertex>& vertices,
	const std::vector<unsigned long>& indices,
	const Material& material
)
	: name(name), 
	vertex_buffer(dev, (int)sizeof(MeshVertex), (int)vertices.size()),
	index_buffer(dev, (int)indices.size()),
	material(material)

{
	CPUBuffer cpu_vertex_buffer(vertices.data(), (int)vertices.size() * (int)sizeof(MeshVertex));
	dev.ScheduleUpload(context, cpu_vertex_buffer, vertex_buffer);
	context.SetTransitionBuffer(vertex_buffer, GPUBufferState::VertexBuffer);

	CPUBuffer cpu_index_buffer(indices.data(), (int)indices.size() * (int)sizeof(unsigned long));
	dev.ScheduleUpload(context, cpu_index_buffer, index_buffer);
	context.SetTransitionBuffer(index_buffer, GPUBufferState::IndexBuffer);
}


void egx::Mesh::BuildAccelerationStructure(Device& dev, CommandContext& context)
{
    context.SetTransitionBuffer(vertex_buffer, GPUBufferState::VertexBuffer | GPUBufferState::NonPixelResource);
    context.SetTransitionBuffer(index_buffer, GPUBufferState::IndexBuffer | GPUBufferState::NonPixelResource);

    D3D12_RAYTRACING_GEOMETRY_DESC geom_desc = {};
    geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geom_desc.Triangles.VertexBuffer.StartAddress = vertex_buffer.view.BufferLocation;
    geom_desc.Triangles.VertexBuffer.StrideInBytes = vertex_buffer.GetElementSize();
    geom_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geom_desc.Triangles.VertexCount = vertex_buffer.GetElementCount();
    geom_desc.Triangles.IndexBuffer = index_buffer.view.BufferLocation;
    geom_desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geom_desc.Triangles.IndexCount = index_buffer.GetElementCount();
    geom_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    if(!material.HasMaskTexture())
        geom_desc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get size required for build
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geom_desc;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    dev.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers.
    blas_scratch = std::make_unique<GPUBuffer>(dev,
        D3D12_RESOURCE_DIMENSION_BUFFER,
        DXGI_FORMAT_UNKNOWN,
        (int)info.ScratchDataSizeInBytes, 1, 1, 1,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        nullptr,
        GPUBufferState::UnorderedAccess);
    blas_result = std::make_unique<GPUBuffer>(dev,
        D3D12_RESOURCE_DIMENSION_BUFFER,
        DXGI_FORMAT_UNKNOWN,
        (int)info.ResultDataMaxSizeInBytes, 1, 1, 1,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        nullptr,
        GPUBufferState::AccelerationStructure);
        

    // Create the bottom-level AS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.Inputs = inputs;
    asDesc.DestAccelerationStructureData = blas_result->buffer->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData = blas_scratch->buffer->GetGPUVirtualAddress();

    context.command_list->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

    context.SetUABarrier(*blas_result);
}