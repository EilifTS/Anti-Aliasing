#include "tlas.h"
#include "../internal/egx_internal.h"
#include "../device.h"
#include "../command_context.h"
#include "../../math/mat4.h"
#include "../mesh.h"

void egx::TLAS::Build(Device& dev, CommandContext& context, std::vector<std::shared_ptr<Model>>& models)
{
    // Count instances
    int instance_count = 0;
    for (auto pmodel : models)
    {
        instance_count += (int)pmodel->GetMeshes().size();
    }

    // First, get the size of the TLAS buffers and create them
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = instance_count;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    dev.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers
    scratch_buffer = std::make_unique<GPUBuffer>(dev,
        D3D12_RESOURCE_DIMENSION_BUFFER,
        DXGI_FORMAT_UNKNOWN,
        (int)info.ScratchDataSizeInBytes, 1, 1, 1,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        nullptr,
        GPUBufferState::UnorderedAccess);
    result_buffer = std::make_unique<GPUBuffer>(dev,
        D3D12_RESOURCE_DIMENSION_BUFFER,
        DXGI_FORMAT_UNKNOWN,
        (int)info.ResultDataMaxSizeInBytes, 1, 1, 1,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        nullptr,
        GPUBufferState::AccelerationStructure);

    // Create instance buffer
    instances_buffer = std::make_unique<UploadHeap>(dev, (int)sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instance_count);

    D3D12_RAYTRACING_INSTANCE_DESC* pInstance_buffer = (D3D12_RAYTRACING_INSTANCE_DESC*)instances_buffer->Map();

    int index = 0;
    for (auto pmodel : models)
    {
        ema::mat4 m = pmodel->CalculateWorldMatrix();
        for (auto pmesh : pmodel->GetMeshes())
        {
            // Initialize the instance desc
            pInstance_buffer[index].InstanceID = 0;
            pInstance_buffer[index].InstanceContributionToHitGroupIndex = 0;
            pInstance_buffer[index].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

            memcpy(pInstance_buffer[index].Transform, &m, sizeof(pInstance_buffer[index].Transform));
            pInstance_buffer[index].AccelerationStructure = pmesh->blas_result->buffer->GetGPUVirtualAddress();
            pInstance_buffer[index].InstanceMask = 0xFF;
            index++;
        }
    }

    // Unmap
    instances_buffer->Unmap();

    // Create the TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC as_desc = {};
    as_desc.Inputs = inputs;
    as_desc.Inputs.InstanceDescs = instances_buffer->buffer->GetGPUVirtualAddress();
    as_desc.DestAccelerationStructureData = result_buffer->buffer->GetGPUVirtualAddress();
    as_desc.ScratchAccelerationStructureData = scratch_buffer->buffer->GetGPUVirtualAddress();

    context.command_list->BuildRaytracingAccelerationStructure(&as_desc, 0, nullptr);

    context.SetUABarrier(*result_buffer);

    // Create shader resource view
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.RaytracingAccelerationStructure.Location = result_buffer->buffer->GetGPUVirtualAddress();

    srv_cpu = dev.buffer_heap->GetNextHandle();
    dev.device->CreateShaderResourceView(nullptr, &srv_desc, srv_cpu);
    srv_gpu = dev.buffer_heap->GetGPUHandle(srv_cpu);
}