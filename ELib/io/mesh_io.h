#pragma once
#include "../graphics/mesh.h"

namespace eio
{
	egx::Mesh LoadMeshFromOBJ(egx::Device& dev, egx::CommandContext& context, const std::string& obj_name, const std::string& mtl_name);
}