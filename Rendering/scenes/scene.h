#pragma once
#include <vector>
#include "graphics/model.h"
#include "graphics/materials.h"

class Scene
{
protected:
	std::vector<std::shared_ptr<egx::Mesh>> meshes;
	std::vector<std::shared_ptr<egx::Model>> static_models;
	std::vector<std::shared_ptr<egx::Model>> dynamic_models;

public:
	Scene() {};
	std::vector<std::shared_ptr<egx::Mesh>> GetMeshes() { return meshes; };
	std::vector<std::shared_ptr<egx::Model>> GetStaticModels() { return static_models; };
	std::vector<std::shared_ptr<egx::Model>> GetDynamicModels() { return dynamic_models; };
	std::vector<std::shared_ptr<egx::Model>> GetModels() 
	{ 
		std::vector<std::shared_ptr<egx::Model>> out(static_models.begin(), static_models.end());
		out.insert(out.end(), dynamic_models.begin(), dynamic_models.end());
		return out;
	};
	virtual void Update(float time) = 0;
};

class SponzaScene : public Scene
{
private:
	std::vector<std::shared_ptr<egx::Mesh>> sponza_mesh;
	std::vector<std::shared_ptr<egx::Mesh>> knight_mesh;
	std::shared_ptr<egx::Model> sponza_model;
	std::shared_ptr<egx::Model> knight_model1;
	std::shared_ptr<egx::Model> knight_model2;
	std::shared_ptr<egx::Model> knight_model3;

public:
	SponzaScene(egx::Device& dev, egx::CommandContext& context, egx::MaterialManager& mat_manager);
	void Update(float time);
};