#include "materials.h"
#include "internal/egx_internal.h"
#include "cpu_buffer.h"
#include "device.h"
#include "command_context.h"
#include "../io/texture_io.h"

void egx::Material::LoadAssets(Device& dev, CommandContext& context)
{
	// Load constant buffer
	const_buffer = std::make_shared<egx::ConstantBuffer>(dev, (int)sizeof(MaterialConstBufferType));

	MaterialConstBufferType mcbt;
	mcbt.diffuse_color = ema::vec4(diffuse_color, 1.0);
	mcbt.specular_exponent = specular_exponent;
	mcbt.use_diffuse_texture = diffuse_map_name != "";

	CPUBuffer cpu_buffer(&mcbt, (int)sizeof(mcbt));
	dev.ScheduleUpload(context, cpu_buffer, *const_buffer);
	context.SetTransitionBuffer(*const_buffer, GPUBufferState::ConstantBuffer);

	// Load diffuse texture
	if (diffuse_map_name != "")
	{
		diffuse_texture = eio::LoadTextureFromFile(dev, context, diffuse_map_name);
		context.SetTransitionBuffer(*diffuse_texture, GPUBufferState::PixelResource);
		diffuse_texture->CreateShaderResourceView(dev);
	}
}

void egx::MaterialManager::AddMaterial(const Material& material)
{
	assert(material_map.find(material.Name()) == material_map.end());

	materials.push_back(material);
	material_map[material.Name()] = &materials.back();
	material_index_map[material.Name()] = (int)materials.size() - 1;
}

void egx::MaterialManager::LoadMaterialAssets(Device& dev, CommandContext& context)
{
	for (auto& m : materials)
		m.LoadAssets(dev, context);
}