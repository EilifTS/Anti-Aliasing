#pragma once
#include "graphics/egx.h"
#include "g_buffer.h"
#include "graphics/camera.h"

class LightManager
{
public:
	LightManager(egx::Device& dev, egx::CommandContext& context);

	void Update(const egx::Camera& player_camera);

	void RenderToShadowMap(egx::Device& dev, egx::CommandContext& context, egx::ModelList& models);
	egx::DepthBuffer& GetShadowMap() { return depth_buffer; };
	egx::ConstantBuffer& GetLightBuffer() { return const_buffer; };

private:
	egx::OrthographicCamera camera;

	ema::vec4 light_dir;
	ema::mat4 view_to_shadowmap_matrix;
	egx::ConstantBuffer const_buffer;
	egx::DepthBuffer depth_buffer;

	egx::RootSignature shadow_rs;
	egx::PipelineState shadow_ps;

};