#pragma once
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

		inline void SetDiffuseColor(const ema::vec3& new_value) { diffuse_color = new_value; };
		inline void SetSpecularExponent(float new_value) { specular_exponent = new_value; };

		void LoadAssets(Device& dev, CommandContext& context);

	private:
		struct MaterialConstBufferType
		{
			ema::vec4 diffuse_color;
			float specular_exponent;
		};

	private:
		std::string material_name;

		ema::vec3 diffuse_color;
		float specular_exponent;

		std::string diffuse_map_name;
		std::string normal_map_name;

		std::shared_ptr<egx::ConstantBuffer> const_buffer;
	};

	class MaterialManager
	{
	public:

		void AddMaterial(const Material& material);

		inline int MaterialCount() const { return (int)materials.size(); };
		inline int GetMaterialIndex(const std::string& material_name) const { return material_index_map.at(material_name); };
		inline const Material& GetMaterial(int material_index) const { return materials.at(material_index); };

		void LoadMaterialAssets(Device& dev, CommandContext& context);

	private:
		std::unordered_map<std::string, egx::Material*> material_map;
		std::unordered_map<std::string, int> material_index_map;
		std::vector<egx::Material> materials;
	};
}