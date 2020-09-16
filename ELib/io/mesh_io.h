#pragma once
#include "../graphics/mesh.h"
#include "../graphics/materials.h"

namespace eio
{
	std::vector<std::shared_ptr<egx::Mesh>> LoadMeshFromOBJ(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager);


	void ConvertOBJToOBJB(const std::string& obj_name);
	std::vector<std::shared_ptr<egx::Mesh>> LoadMeshFromOBJB(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, egx::MaterialManager& mat_manager);
}