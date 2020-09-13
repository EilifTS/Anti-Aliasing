#pragma once
#include "../graphics/mesh.h"
#include "../graphics/materials.h"

namespace eio
{
	std::vector<egx::Mesh> LoadMeshFromOBJ(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager);
}