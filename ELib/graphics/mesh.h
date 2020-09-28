#pragma once
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "input_layout.h"
#include "../math/color.h"
#include <string>
#include <vector>
#include "materials.h"
#include "../io/texture_io.h"

namespace egx
{
	struct MeshVertex
	{
		ema::vec3 position;
		ema::vec3 normal;
		ema::vec4 tangent;
		ema::vec2 tex_coord;

		static InputLayout GetInputLayout()
		{
			InputLayout out;
			out.AddPosition(3);
			out.AddNormal(3);
			out.AddNormal(4);
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

		inline const VertexBuffer& GetVertexBuffer() const { return vertex_buffer; };
		inline VertexBuffer& GetVertexBuffer() { return vertex_buffer; };
		inline const IndexBuffer& GetIndexBuffer() const { return index_buffer; };
		inline IndexBuffer& GetIndexBuffer() { return index_buffer; };
		inline const Material& GetMaterial() const { return material; };

		void BuildAccelerationStructure(Device& dev, CommandContext& context);

	private:
		VertexBuffer vertex_buffer;
		IndexBuffer index_buffer;
		std::string name;
		const Material& material;

		// Ray tracing
		std::unique_ptr<GPUBuffer> blas_scratch;
		std::unique_ptr<GPUBuffer> blas_result;

	private:
		friend TLAS;
	};
}