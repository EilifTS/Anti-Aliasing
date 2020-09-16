#include "texture_io.h"
#include "../graphics/internal/egx_internal.h"
#include "internal/WICTextureLoader12.h"
#include "../graphics/device.h"
#include "../graphics/command_context.h"
#include "../graphics/cpu_buffer.h"

namespace
{
	std::vector<uint8_t> generateMipmaps(uint8_t* data, int width, int height, int bytes_per_pixel)
	{
		// Calculate mipmapped data size
		int buffer_size = width * height * bytes_per_pixel;
		int mip_levels = 0;
		int current_width = width;
		int current_height = height;
		while (current_width != 1 || current_height != 1)
		{
			if (current_width != 1)
				current_width >>= 1;
			if (current_height != 1)
				current_height >>= 1;
			buffer_size += current_width * current_height * bytes_per_pixel;
			mip_levels++;
		}

		std::vector<uint8_t> data_out(buffer_size);

		// Start by copying over start data
		for (int i = 0; i < width * height * bytes_per_pixel; i++)
			data_out[i] = data[i];

		int mip_offset_u = 0;
		int mip_offset_s = width * height * bytes_per_pixel;

		int width_u = width;
		int height_u = height;
		int width_s = width >> 1;
		int height_s = height >> 1;
		for (int m = 0; m < mip_levels; m++)
		{
			for (int y = 0; y < height_s; y++)
			{
				for (int x = 0; x < width_s; x++)
				{
					for (int i = 0; i < bytes_per_pixel; i++)
					{
						int index_u0 = mip_offset_u + ((2 * y + 0)             * width_u + 2 * x + 0)              * bytes_per_pixel + i;
						int index_u1 = mip_offset_u + ((2 * y + 0)             * width_u + 2 * x + (1 % height_u)) * bytes_per_pixel + i;
						int index_u2 = mip_offset_u + ((2 * y + (1 % width_u)) * width_u + 2 * x + 0)              * bytes_per_pixel + i;
						int index_u3 = mip_offset_u + ((2 * y + (1 % width_u)) * width_u + 2 * x + (1 % height_u)) * bytes_per_pixel + i;
						int index_s = mip_offset_s + (y * width_s + x) * bytes_per_pixel + i;
						data_out[index_s] =
							(uint8_t)((
								(uint32_t)data_out[index_u0] + 
								(uint32_t)data_out[index_u1] + 
								(uint32_t)data_out[index_u2] + 
								(uint32_t)data_out[index_u3]
								) >> 2
								);
					}
				}
			}
			width_u = width_s;
			height_u = height_s;
			if (width_s != 1)
				width_s >>= 1;
			if (height_s != 1)
				height_s >>= 1;

			mip_offset_u = mip_offset_s;
			mip_offset_s += width_u * height_u * bytes_per_pixel;
		}

		return data_out;
	}
}

std::shared_ptr<egx::Texture2D> eio::LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name, bool use_srgb)
{
	std::wstring wfile_name(file_name.begin(), file_name.end());
	ComPtr<ID3D12Resource> texture_buffer;
	std::unique_ptr<uint8_t[]> data;
	D3D12_SUBRESOURCE_DATA sub_data;

	unsigned int flags = DirectX::WIC_LOADER_MIP_AUTOGEN;
	if (use_srgb)
		flags |= DirectX::WIC_LOADER_FORCE_SRGB;
	else 
		flags |= DirectX::WIC_LOADER_IGNORE_SRGB;

	THROWIFFAILED(
		DirectX::LoadWICTextureFromFileEx(
			dev.device.Get(), 
			wfile_name.c_str(), 
			0,
			D3D12_RESOURCE_FLAG_NONE,
			flags,
			&texture_buffer,
			data,
			sub_data
		),
		"Failed to load file" + file_name
	);

	std::shared_ptr<egx::Texture2D> texture = std::shared_ptr<egx::Texture2D>(new egx::Texture2D(texture_buffer, D3D12_RESOURCE_STATE_COPY_DEST));

	auto mipped_data = generateMipmaps(data.get(), texture->Size().x, texture->Size().y, texture->GetElementSize());

	egx::CPUBuffer cpu_buffer(mipped_data.data(), (int)mipped_data.size());
	dev.ScheduleUpload(context, cpu_buffer, *texture);

	return texture;
}