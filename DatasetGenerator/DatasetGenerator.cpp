#include "../ELib/graphics/device.h"
#include "../ELib/graphics/command_context.h"
#include "../ELib/math/point2d.h"
#include "../ELib/io/game_clock.h"
#include "../ELib/io/console.h"

namespace
{
	static const ema::point2D output_size = ema::point2D(1920, 1080);
}

int main()
{
	eio::GameClock clock;
	eio::Console::InitConsole2(&clock);
	// Create device and context
	egx::Device device;
	egx::CommandContext context(device, output_size);

	// Create scene
	// Create camera
	// Create different AA-modes

	// Load camera positions
	// Setup the different render options
	// Loop over camera positions and save images


}
