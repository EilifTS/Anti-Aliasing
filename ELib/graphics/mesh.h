#pragma once
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "input_layout.h"
#include "../math/color.h"
#include <string>
#include <vector>
#include "materials.h"

namespace egx
{
	struct NormalMappedVertex
	{
		ema::vec3 position;
		ema::vec3 normal;
		ema::vec3 tangent;
		ema::vec3 bitangent;
		ema::vec2 tex_coord;

		static InputLayout GetInputLayout()
		{
			InputLayout out;
			out.AddPosition(3);
			out.AddNormal(3);
			out.AddNormal(3);
			out.AddNormal(3);
			out.AddTextureCoordinate(2);
			return out;
		}
	};

	struct MeshVertex
	{
		ema::vec3 position;
		ema::vec3 normal;
		ema::vec2 tex_coord;

		static InputLayout GetInputLayout()
		{
			InputLayout out;
			out.AddPosition(3);
			out.AddNormal(3);
			out.AddTextureCoordinate(2);
			return out;
		}
	};

	class Mesh
	{
	public:
		Mesh(
			Device& dev, 
			CommandContext& context, 
			const std::string& name, 
			const std::vector<MeshVertex>& vertices, 
			const std::vector<unsigned long>& indices,
			const Material& material
		);
		Mesh(
			Device& dev,
			CommandContext& context,
			const std::string& name,
			const std::vector<NormalMappedVertex>& vertices,
			const std::vector<unsigned long>& indices,
			const Material& material
		);

		inline const VertexBuffer& GetVertexBuffer() const { return vertex_buffer; };
		inline VertexBuffer& GetVertexBuffer() { return vertex_buffer; };
		inline const IndexBuffer& GetIndexBuffer() const { return index_buffer; };
		inline IndexBuffer& GetIndexBuffer() { return index_buffer; };
		inline const Material& GetMaterial() const { return material; };

	private:
		VertexBuffer vertex_buffer;
		IndexBuffer index_buffer;
		std::string name;
		const Material& material;
	};

	typedef std::vector<std::shared_ptr<Mesh>> ModelList;
	class ModelManager
	{
	public:
		ModelManager();

		void LoadMesh(Device& dev, CommandContext& context, const std::string& file_path);
		void LoadAssets(Device& dev, CommandContext& context);

		inline ModelList& GetNormalModels() { return meshes; };
		inline ModelList& GetNMModels() { return norm_mapped_meshes; };

	private:
		MaterialManager mat_manager;

		ModelList meshes;
		ModelList norm_mapped_meshes;

	};
}