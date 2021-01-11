#include "graphics/device.h"
#include "graphics/command_context.h"
#include "graphics/materials.h"
#include "graphics/camera.h"
#include "math/point2d.h"
#include "math/vec2.h"
#include "io/game_clock.h"
#include "io/console.h"
#include "io/texture_io.h"
#include "scenes/scene.h"
#include "deferred_rendering/deferred_renderer.h"
#include "network/dataset_video_recorder.h"
#include "aa/ssaa/ssaa.h"
#include "misc/string_helpers.h"

#pragma comment(lib, "runtimeobject.lib")
#include <wrl/wrappers/corewrappers.h>
#include <stdio.h>
#include <io.h>

namespace
{
	static const ema::point2D output_size = ema::point2D(1920, 1080);

	static const float near_plane = 0.1f;
	static const float far_plane = 1000.0f;

	static const int super_sample_options[] = { 32 };
	static const int upsample_factor_options[] = { 2, 3, 4 };

}

void renderRasterizer(egx::Device& dev, egx::CommandContext& context, Scene& scene, DeferredRenderer& renderer, egx::Camera& camera, egx::RenderTarget& target)
{
	auto models = scene.GetModels();
	auto static_models = scene.GetStaticModels();
	auto dynamic_models = scene.GetDynamicModels();

	for (auto pmodel : models) pmodel->UpdateBuffer(dev, context);
	renderer.PrepareFrame(dev, context);
	for (auto pmodel : static_models) renderer.RenderModel(dev, context, camera, *pmodel);
	for (auto pmodel : dynamic_models) renderer.RenderModel(dev, context, camera, *pmodel);
	for (auto pmodel : dynamic_models) renderer.RenderMotionVectors(dev, context, camera, *pmodel);
	renderer.RenderLight(dev, context, camera, target);
	renderer.PrepareFrameEnd();
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

	// Prepare camera videos
	int video_count = enn::LoadDatasetCount();
	enn::DatasetVideo video;

	// Make folder for all data
	std::string common_dir = "data";
	CreateDirectoryA(common_dir.c_str(), NULL);
	

	{ // SSAA images

		// Create resolution dependent resources
		DeferredRenderer renderer(device, context, output_size, far_plane);
		egx::FPCamera camera(device, context, (ema::vec2)output_size, near_plane, far_plane, 3.141592f / 3.0f, 0.0f, 0.0f);
		egx::RenderTarget target1(device, egx::TextureFormat::UNORM8x4, output_size);
		egx::RenderTarget target2(device, egx::TextureFormat::UNORM8x4, output_size);
		egx::RenderTarget target3(device, egx::TextureFormat::UNORM8x4SRGB, output_size);
		target1.CreateShaderResourceView(device);
		target1.CreateRenderTargetView(device);
		target2.CreateShaderResourceView(device);
		target2.CreateRenderTargetView(device);
		target3.CreateShaderResourceView(device);
		target3.CreateRenderTargetView(device);

		for (int ssaa_spp : super_sample_options)
		{
			// Prepare SSAA
			SSAA ssaa(device, output_size, ssaa_spp);

			eio::Console::Log("Processing images with " + emisc::ToString(ssaa_spp) + " samples per pixel");

			// Make folder for all images with current spp
			std::string directory_name = common_dir + "/spp" + emisc::ToString(ssaa_spp);
			CreateDirectoryA(directory_name.c_str(), NULL);

			for (int video_index = 0; video_index < video_count; video_index++)
			{
				video.LoadFromFile(video_index);

				// Make folder for all images in this video
				std::string video_directory_name = directory_name + "/video" + emisc::ToString(video_index);
				CreateDirectoryA(video_directory_name.c_str(), NULL);

				int frame_count = (int)video.frames.size();
				for(int frame_index = 0; frame_index < frame_count; frame_index++)
				{
					auto& frame = video.frames[frame_index];

					context.SetDescriptorHeap(*device.buffer_heap);

					camera.SetPosition(frame.camera_position);
					camera.SetRotation(frame.camera_rotation);
					camera.Update();
					scene.Update((float)frame.time);
					renderer.UpdateLight(camera);

					eio::Console::LogProgress("Processing frame " + emisc::ToString(frame_index + video_index * frame_count) + "/" + emisc::ToString(video_count * frame_count));
					ssaa.PrepareForRender(context);
					for (int i = 0; i < ssaa.GetSampleCount(); i++)
					{
						camera.SetJitter(ssaa.GetJitter());
						camera.Update();
						camera.UpdateBuffer(device, context);
						renderRasterizer(device, context, scene, renderer, camera, target1);
						ssaa.AddSample(context, target1);
					}
					ssaa.Finish(context, target2);

					renderer.ApplyToneMapping(device, context, target2, target3);

					device.QueueList(context);
					device.WaitForGPU();

					eio::SaveTextureToFile(device, context, target3,
						video_directory_name + 
						"/spp" + emisc::ToString(ssaa_spp) +
						"_v" + emisc::ToString(video_index) +
						"_f" + emisc::ToString(frame_index) + ".png");
				}
			}
		}

	}

	// Downsampled images
	for (int upsampling_factor : upsample_factor_options)
	{
		ema::point2D input_resolution = output_size / upsampling_factor;
		// Create resolution dependent resources
		DeferredRenderer renderer(device, context, input_resolution, far_plane);
		egx::FPCamera camera(device, context, (ema::vec2)input_resolution, near_plane, far_plane, 3.141592f / 3.0f, 0.0f, 0.0f);
		egx::RenderTarget target1(device, egx::TextureFormat::UNORM8x4, input_resolution);
		egx::RenderTarget target2(device, egx::TextureFormat::UNORM8x4SRGB, input_resolution);
		target1.CreateShaderResourceView(device);
		target1.CreateRenderTargetView(device);
		target2.CreateShaderResourceView(device);
		target2.CreateRenderTargetView(device);

		eio::Console::Log("Processing images with a resolution of " + emisc::ToString(input_resolution.x) + "x" + emisc::ToString(input_resolution.y));

		// Make folders for all images with current spp
		std::string directory_name = common_dir + "/us" + emisc::ToString(upsampling_factor);
		std::string image_directory_name = directory_name + "/images";
		std::string depth_directory_name = directory_name + "/depth";
		std::string mv_directory_name = directory_name + "/motion_vectors";
		CreateDirectoryA(directory_name.c_str(), NULL);
		CreateDirectoryA(image_directory_name.c_str(), NULL);
		CreateDirectoryA(depth_directory_name.c_str(), NULL);
		CreateDirectoryA(mv_directory_name.c_str(), NULL);

		for (int video_index = 0; video_index < video_count; video_index++)
		{
			video.LoadFromFile(video_index);

			// Make folder for all images in this video
			std::string image_video_directory_name = image_directory_name + "/video" + emisc::ToString(video_index);
			std::string depth_video_directory_name = depth_directory_name + "/video" + emisc::ToString(video_index);
			std::string mv_video_directory_name = mv_directory_name + "/video" + emisc::ToString(video_index);
			CreateDirectoryA(image_video_directory_name.c_str(), NULL);
			CreateDirectoryA(depth_video_directory_name.c_str(), NULL);
			CreateDirectoryA(mv_video_directory_name.c_str(), NULL);

			int frame_count = (int)video.frames.size();
			for (int frame_index = 0; frame_index < frame_count; frame_index++)
			{
				auto& frame = video.frames[frame_index];

				context.SetDescriptorHeap(*device.buffer_heap);

				camera.SetPosition(frame.camera_position);
				camera.SetRotation(frame.camera_rotation);
				//camera.SetJitter(ssaa.GetJitter());
				camera.Update();
				scene.Update((float)frame.time);
				renderer.UpdateLight(camera);

				eio::Console::LogProgress("Processing frame " + emisc::ToString(frame_index + video_index * frame_count) + "/" + emisc::ToString(video_count * frame_count));
				
				camera.UpdateBuffer(device, context);
				renderRasterizer(device, context, scene, renderer, camera, target1);
				
				renderer.ApplyToneMapping(device, context, target1, target2);

				device.QueueList(context);
				device.WaitForGPU();

				eio::SaveTextureToFile(device, context, target2,
					image_video_directory_name +
					"/image_us" + emisc::ToString(upsampling_factor) +
					"_v" + emisc::ToString(video_index) +
					"_f" + emisc::ToString(frame_index) + ".png");

				eio::SaveTextureToFileDDS(device, context, renderer.GetGBuffer().DepthBuffer(),
					depth_video_directory_name +
					"/depth_us" + emisc::ToString(upsampling_factor) +
					"_v" + emisc::ToString(video_index) +
					"_f" + emisc::ToString(frame_index) + ".dds");

				eio::SaveTextureToFileDDS(device, context, renderer.GetMotionVectors(),
					mv_video_directory_name +
					"/motion_vectors_us" + emisc::ToString(upsampling_factor) +
					"_v" + emisc::ToString(video_index) +
					"_f" + emisc::ToString(frame_index) + ".dds");
			}
		}
	}
		



}