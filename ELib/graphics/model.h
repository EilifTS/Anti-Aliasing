#pragma once
#include "internal/egx_common.h"
#include "constant_buffer.h"
#include <vector>
#include <memory>
#include "../math/mat4.h"

namespace egx
{
	class Model
	{
	public:
		Model(Device& dev, const std::vector<std::shared_ptr<Mesh>>& meshes);
		void UpdateBuffer(Device& dev, CommandContext& context);

		inline std::vector<std::shared_ptr<Mesh>>& GetMeshes() { return meshes; };
		inline ConstantBuffer& GetModelBuffer() { return model_buffer; };

		inline bool IsStatic() const { return is_static; };
		inline const ema::vec3& Position() const { return position; };
		inline const ema::vec3& Rotation() const { return rotation; };
		inline const ema::vec3& Scale() const { return scale; };

		inline void SetStatic(bool new_static_val) { is_static = new_static_val; };
		inline void SetPosition(const ema::vec3& new_pos) { position = new_pos; };
		inline void SetRotation(const ema::vec3& new_rot) { rotation = new_rot; };
		inline void SetScale(const ema::vec3& new_scale) { scale = new_scale; };
		inline void SetScale(float new_scale) { scale = ema::vec3(new_scale); };


	private:
		std::vector<std::shared_ptr<Mesh>> meshes;
		ConstantBuffer model_buffer;

		bool is_static;

		ema::vec3 position;
		ema::vec3 rotation;
		ema::vec3 scale;
		ema::mat4 last_world_matrix;

	};
}