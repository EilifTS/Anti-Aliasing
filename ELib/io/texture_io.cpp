#include "texture_io.h"
#include "../graphics/internal/egx_internal.h"
#include "internal/WICTextureLoader12.h"
#include "../graphics/device.h"
#include "../graphics/command_context.h"
#include "../graphics/cpu_buffer.h"


std::shared_ptr<egx::Texture2D> eio::LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name)
{
	std::wstring wfile_name(file_name.begin(), file_name.end());
	ComPtr<ID3D12Resource> texture_buffer;
	std::unique_ptr<uint8_t[]> data;
	D3D12_SUBRESOURCE_DATA sub_data;

	THROWIFFAILED(
		LoadWICTextureFromFileEx(
			dev.device.Get(), 
			wfile_name.c_str(), 
			0,
			D3D12_RESOURCE_FLAG_NONE,
			DirectX::WIC_LOADER_DEFAULT,
			&texture_buffer,
			data,
			sub_data
		),
		"Failed to load file" + file_name
	);

	std::shared_ptr<egx::Texture2D> texture = std::shared_ptr<egx::Texture2D>(new egx::Texture2D(texture_buffer, D3D12_RESOURCE_STATE_COPY_DEST));
	egx::CPUBuffer cpu_buffer(sub_data.pData, (int)sub_data.SlicePitch);
	dev.ScheduleUpload(context, cpu_buffer, *texture);

	return texture;
}