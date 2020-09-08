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
		ema::vec3 color;
		float specular_exp;

		static InputLayout GetInputLayout()
		{
			InputLayout out;
			out.AddPosition(3);
			out.AddNormal(3);
			out.AddColor(3);
			out.AddTextureCoordinate(1);
			return out;
		}
	};
	struct Material
	{
		ema::vec3 diffuse_color;
		float specular_exponent;
	};

	class Mesh
	{
	public:
		Mesh(
			Device& dev, 
			CommandContext& context, 
			const std::string& name, 
			const std::vector<MeshVertex>& vertices, 
			const std::vector<unsigned long>& indices
		);

		inline VertexBuffer& GetVertexBuffer() { return vertex_buffer; };
		inline IndexBuffer& GetIndexBuffer() { return index_buffer; };

	private:
		VertexBuffer vertex_buffer;
		IndexBuffer index_buffer;
		std::string name;
	};
}