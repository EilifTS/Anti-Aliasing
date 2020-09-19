#include "materials.h"
#include "internal/egx_internal.h"
#include "cpu_buffer.h"
#include "device.h"
#include "command_context.h"
#include "../io/texture_io.h"

void egx::Material::LoadAssets(Device& dev, CommandContext& context, eio::TextureLoader& texture_loader)
{
	// Load constant buffer
	const_buffer = std::make_shared<egx::ConstantBuffer>(dev, (int)sizeof(MaterialConstBufferType));

	MaterialConstBufferType mcbt;
	mcbt.diffuse_color = ema::vec4(diffuse_color, 1.0);
	mcbt.specular_exponent = specular_exponent;
	mcbt.use_diffuse_texture = (int)HasDiffuseTexture();
	mcbt.use_normal_map = (int)HasNormalMap();
	mcbt.use_specular_map = (int)HasSpecularMap();
	mcbt.use_mask_texture = (int)HasMaskTexture();

	CPUBuffer cpu_buffer(&mcbt, (int)sizeof(mcbt));
	dev.ScheduleUpload(context, cpu_buffer, *const_buffer);
	context.SetTransitionBuffer(*const_buffer, GPUBufferState::ConstantBuffer);

	// Load diffuse texture
	if (HasDiffuseTexture())
	{
		diffuse_texture = texture_loader.LoadTexture(dev, context, diffuse_map_name);
		context.SetTransitionBuffer(*diffuse_texture, GPUBufferState::PixelResource);
		diffuse_texture->CreateShaderResourceView(dev);
	}

	// Load normal map
	if (HasNormalMap())
	{
		normal_map = texture_loader.LoadTexture(dev, context, normal_map_name, false);
		context.SetTransitionBuffer(*normal_map, GPUBufferState::PixelResource);
		normal_map->CreateShaderResourceView(dev);
	}

	// Load specular map
	if (HasSpecularMap())
	{
		specular_map = texture_loader.LoadTexture(dev, context, specular_map_name, false);
		context.SetTransitionBuffer(*specular_map, GPUBufferState::PixelResource);
		specular_map->CreateShaderResourceView(dev);
	}

	// Load mask texture
	if (HasMaskTexture())
	{
		mask_texture = texture_loader.LoadTexture(dev, context, mask_texture_name, false);
		context.SetTransitionBuffer(*mask_texture, GPUBufferState::PixelResource);
		mask_texture->CreateShaderResourceView(dev);
	}
}

void egx::MaterialManager::AddMaterial(const Material& material)
{
	assert(material_map.find(material.Name()) == material_map.end());

	materials.push_back(std::make_shared<Material>(material));
	material_map[material.Name()] = materials.back();
	material_index_map[material.Name()] = (int)materials.size() - 1;
}

void egx::MaterialManager::DisableDiffuseTextures()
{
	for (auto pm : materials)
		pm->SetDiffuseMapName("");
}
void egx::MaterialManager::DisableNormalMaps()
{
	for (auto pm : materials)
		pm->SetNormalMapName("");
}
void egx::MaterialManager::DisableSpecularMaps()
{
	for (auto pm : materials)
		pm->SetSpecularMapName("");
}
void egx::MaterialManager::DisableMaskTextures()
{
	for (auto pm : materials)
		pm->SetMaskTextureName("");
}

void egx::MaterialManager::LoadMaterialAssets(Device& dev, CommandContext& context, eio::TextureLoader& texture_loader)
{
	for (auto pm : materials)
		pm->LoadAssets(dev, context, texture_loader);
}