#pragma once
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "input_layout.h"
#include "../math/color.h"
#include <string>
#include <vector>

namespace egx
{
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
			out.AddIntegers(1);
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
			int material_index
		);

		inline VertexBuffer& GetVertexBuffer() { return vertex_buffer; };
		inline IndexBuffer& GetIndexBuffer() { return index_buffer; };
		inline int GetMaterialIndex() const { return material_index; };

	private:
		VertexBuffer vertex_buffer;
		IndexBuffer index_buffer;
		std::string name;
		int material_index;
	};
}