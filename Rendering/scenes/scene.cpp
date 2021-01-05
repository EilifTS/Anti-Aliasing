#include "scene.h"
#include "io/mesh_io.h"


SponzaScene::SponzaScene(egx::Device& dev, egx::CommandContext& context, egx::MaterialManager& mat_manager)
{
	// Load assets
	sponza_mesh = eio::LoadMeshFromOBJB(dev, context, "../Rendering/models/sponza", mat_manager);
	sponza_model = std::make_shared<egx::Model>(dev, sponza_mesh);
	sponza_model->SetScale(0.01f);
	sponza_model->SetStatic(true);

	knight_mesh = eio::LoadMeshFromOBJB(dev, context, "../Rendering/models/knight", mat_manager);
	knight_model1 = std::make_shared<egx::Model>(dev, knight_mesh);
	knight_model2 = std::make_shared<egx::Model>(dev, knight_mesh);
	knight_model3 = std::make_shared<egx::Model>(dev, knight_mesh);
	knight_model1->SetScale(1.2f);
	knight_model2->SetScale(1.2f);
	knight_model3->SetScale(1.2f);
	knight_model1->SetPosition(ema::vec3(-0.8f, 0.0f, 0.5f));
	knight_model2->SetPosition(ema::vec3(-4.4f, 0.0f, 0.5f));
	knight_model3->SetPosition(ema::vec3(3.0f, 0.0f, 0.5f));
	knight_model1->SetRotation(ema::vec3(0.0f, 0.0f, 0.0f));
	knight_model2->SetRotation(ema::vec3(0.0f, 0.0f, 3.141692f));
	knight_model3->SetRotation(ema::vec3(0.0f, 0.0f, 3.141692f));
	//knight_model3->SetStatic(true);

	meshes.insert(meshes.end(), sponza_mesh.begin(), sponza_mesh.end());
	meshes.insert(meshes.end(), knight_mesh.begin(), knight_mesh.end());
	static_models.push_back(sponza_model);
	dynamic_models.push_back(knight_model1);
	dynamic_models.push_back(knight_model2);
	dynamic_models.push_back(knight_model3);
}

void SponzaScene::Update(float time)
{
	float rot = time;
	knight_model1->SetRotation(ema::vec3(0.0f, 0.0f, rot));
	knight_model2->SetPosition(ema::vec3(-4.4f, 0.0f, 0.5f + 0.5f * sinf(10.0f * time)));
}