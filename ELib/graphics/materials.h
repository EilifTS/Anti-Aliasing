#pragma once
#include "../io/texture_io.h"
#include "internal/egx_common.h"
#include "constant_buffer.h"
#include "../math/color.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace egx
{
	class Material
	{
	public:
		Material() : material_name(""), specular_exponent(0.0f), const_buffer(nullptr) {};
		Material(const std::string& name) : material_name(name), specular_exponent(0.0f), const_buffer(nullptr) {};

		inline const std::string& Name() const { return material_name; };
		inline const ConstantBuffer& GetBuffer() const { return *const_buffer; };

		inline bool HasDiffuseTexture() const { return diffuse_map_name != ""; };
		inline bool HasNormalMap() const { return normal_map_name != ""; };
		inline bool HasSpecularMap() const { return specular_map_name != ""; };
		inline bool HasMaskTexture() const { return mask_texture_name != ""; };
		inline const Texture2D& GetDiffuseTexture() const { return *diffuse_texture; };
		inline const Texture2D& GetNormalMap() const { return *normal_map; };
		inline const Texture2D& GetSpecularMap() const { return *specular_map; };
		inline const Texture2D& GetMaskTexture() const { return *mask_texture; };

		inline void SetDiffuseColor(const ema::vec3& new_value) { diffuse_color = new_value; };
		inline void SetSpecularExponent(float new_value) { specular_exponent = new_value; };
		inline void SetReflectance(float new_value) { reflectance = new_value; };
		inline void SetDiffuseMapName(const std::string& new_name) { diffuse_map_name = new_name; }
		inline void SetNormalMapName(const std::string& new_name) { normal_map_name = new_name; }
		inline void SetSpecularMapName(const std::string& new_name) { specular_map_name = new_name; }
		inline void SetMaskTextureName(const std::string& new_name) { mask_texture_name = new_name; }

		void LoadAssets(Device& dev, CommandContext& context, eio::TextureLoader& texture_loader);

	private:
		struct MaterialConstBufferType
		{
			ema::vec4 diffuse_color;
			float specular_exponent;
			float reflectance = 0.0f;
			int use_diffuse_texture;
			int use_normal_map;
			int use_specular_map;
			int use_mask_texture;
		};

	private:
		std::string material_name;

		ema::vec3 diffuse_color;
		float specular_exponent;
		float reflectance;

		std::string diffuse_map_name;
		std::string normal_map_name;
		std::string specular_map_name;
		std::string mask_texture_name;

		std::shared_ptr<egx::ConstantBuffer> const_buffer;
		std::shared_ptr<egx::Texture2D> diffuse_texture;
		std::shared_ptr<egx::Texture2D> normal_map;
		std::shared_ptr<egx::Texture2D> specular_map;
		std::shared_ptr<egx::Texture2D> mask_texture;
	};

	class MaterialManager
	{
	public:

		void AddMaterial(const Material& material);

		inline int MaterialCount() const { return (int)materials.size(); };
		inline int GetMaterialIndex(const std::string& material_name) const { return material_index_map.at(material_name); };
		inline const Material& GetMaterial(int material_index) const { return *materials.at(material_index); };

		void DisableDiffuseTextures();
		void DisableNormalMaps();
		void DisableSpecularMaps();
		void DisableMaskTextures();
		void LoadMaterialAssets(Device& dev, CommandContext& context, eio::TextureLoader& texture_loader);

	private:
		std::unordered_map<std::string, std::shared_ptr<egx::Material>> material_map;
		std::unordered_map<std::string, int> material_index_map;
		std::vector<std::shared_ptr<egx::Material>> materials;
	};
}