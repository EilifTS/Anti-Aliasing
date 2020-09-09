#pragma once
#include <memory>

#include "graphics/device.h"
#include "graphics/command_context.h"
#include "io/input_manager.h"

#include "graphics/vertex_buffer.h"
#include "graphics/root_signature.h"
#include "graphics/pipeline_state.h"
#include "graphics/mesh.h"
#include "graphics/camera.h"
#include "graphics/depth_buffer.h"

class App
{
public:
	App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

	void Update(eio::InputManager& im);
	void Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

private:

	egx::FPCamera camera;
	egx::Mesh mesh;
	egx::DepthBuffer depth_buffer;
	egx::RootSignature root_sig;
	egx::PipelineState pipeline_state;
};