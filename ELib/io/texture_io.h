#pragma once
#include "../graphics/texture2d.h"
#include <string>
#include <unordered_map>

namespace eio
{
	std::shared_ptr<egx::Texture2D> LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name, bool use_srgb);

	void SaveTextureToFile(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, const std::string& file_name);
	void SaveTextureToFileDDS(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, const std::string& file_name);

	class TextureLoader
	{
	public:
		TextureLoader() {};

		std::shared_ptr<egx::Texture2D> LoadTexture(egx::Device& dev, egx::CommandContext& context, const std::string& file_name, bool use_srgb = true)
		{
			if (textures.find(file_name) == textures.end())
			{
				// Load file
				textures[file_name] = LoadTextureFromFile(dev, context, file_name, use_srgb);
			}
			else
			{
				// File allready loaded, so just retrive it from map
			}
			return textures[file_name];
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<egx::Texture2D>> textures;
	};
}