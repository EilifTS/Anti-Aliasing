#pragma once
#include "../graphics/texture2d.h"
#include <string>


namespace eio
{
	std::shared_ptr<egx::Texture2D> LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name);

	
}