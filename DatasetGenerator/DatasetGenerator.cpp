#include "graphics/device.h"
#include "graphics/command_context.h"
#include "graphics/materials.h"
#include "math/point2d.h"
#include "io/game_clock.h"
#include "io/console.h"
#include "io/texture_io.h"
#include "scenes/scene.h"

#pragma comment(lib, "runtimeobject.lib")
#include <wrl/wrappers/corewrappers.h>

namespace
{
	static const ema::point2D output_size = ema::point2D(1920, 1080);
}

int main()
{
	// Initialize windows runtime
	// Needed for texture saving and loading to work
	Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
	if (FAILED(initialize))
	{
		eio::Console::Log("Failed to initialize windows runtime");
		return 1;
	}

	eio::GameClock clock;
	eio::Console::InitConsole2(&clock);
	// Create device and context
	egx::Device device;
	egx::CommandContext context(device, output_size);

	// Create scene
	egx::MaterialManager mat_manager;
	eio::TextureLoader texture_loader;
	SponzaScene scene(device, context, mat_manager);
	mat_manager.LoadMaterialAssets(device, context, texture_loader);

	// Create camera
	// Create different AA-modes

	// Load camera positions
	// Setup the different render options
	// Loop over camera positions and save images


}
