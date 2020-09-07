#pragma once
#include <memory>

#include "graphics/device.h"
#include "graphics/command_context.h"
#include "io/input_manager.h"

#include "graphics/vertex_buffer.h"
#include "graphics/root_signature.h"
#include "graphics/pipeline_state.h"

class App
{
public:
	App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

	void Update(eio::InputManager& im);
	void Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

private:

	std::unique_ptr<egx::VertexBuffer> vertex_buffer;
	egx::RootSignature root_sig;
	egx::PipelineState pipeline_state;
};