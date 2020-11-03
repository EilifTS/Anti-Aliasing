#include "dataset_video_recorder.h"
#include <fstream>
#include <stdexcept>
#include "../misc/string_helpers.h"
#include "../io/console.h"

namespace
{
	static const std::string dataset_folder_name = "dataset/";
	static const std::string dataset_video_filename = "dataset_video";
	static const std::string dataset_count_filename = "dataset_video_count.txt";
}

/*
	File format:
	{Number of frames}
	{time0} {cam_pos0} {cam_rot0}
	{time1} {cam_pos1} {cam_rot1}
	{time2} {cam_pos2} {cam_rot2}
	...
*/

void enn::DatasetVideo::SaveToFile(int file_nr)
{
	std::string filename = dataset_folder_name + dataset_video_filename + emisc::ToString(file_nr) + ".txt";

	std::ofstream file(filename);
	if (file.fail())
		throw std::runtime_error("Failed to save file " + filename);

	file << frames.size() << std::endl;
	for (const auto& frame : frames)
	{
		file << frame.time << " " <<
			frame.camera_position.x << " " << frame.camera_position.y << " " << frame.camera_position.z << " " <<
			frame.camera_rotation.x << " " << frame.camera_rotation.y << " " << frame.camera_rotation.z << std::endl;
	}
}

void enn::DatasetVideo::LoadFromFile(int file_nr)
{
	std::string filename = dataset_folder_name + dataset_video_filename + emisc::ToString(file_nr) + ".txt";

	std::ifstream file(filename);
	if (file.fail())
		throw std::runtime_error("Failed to load file " + filename);

	int frame_count = 0;
	file >> frame_count;
	frames.resize(frame_count);

	for (int i = 0; i < frame_count; i++)
	{
		auto& frame = frames[i];
		file >> frame.time >>
			frame.camera_position.x >> frame.camera_position.y >> frame.camera_position.z >>
			frame.camera_rotation.x >> frame.camera_rotation.y >> frame.camera_rotation.z;
	}
}


enn::DatasetVideoRecorder::DatasetVideoRecorder()
{
	dataset_count = LoadDatasetCount();
}

void enn::DatasetVideoRecorder::StartRecording(int frame_count)
{
	dataset_video.frames.resize(frame_count);
	current_frame = 0;
	max_frame_count = frame_count;
	is_ready = false;

	eio::Console::Log("Starting dataset recording");
}
void enn::DatasetVideoRecorder::CaptureFrame(const enn::DatasetFrame& frame)
{
	dataset_video.frames[current_frame++] = frame;
	if (current_frame == max_frame_count)
	{
		is_ready = true;
		dataset_video.SaveToFile(dataset_count++);
		saveDatasetCount();
		eio::Console::Log("Saving dataset recording");
	}
}

bool enn::DatasetVideoRecorder::IsReady()
{
	return is_ready;
}

void enn::DatasetVideoRecorder::saveDatasetCount()
{
	std::ofstream file(dataset_folder_name + dataset_count_filename);
	if (file.fail())
		throw std::runtime_error("Failed to save file " + dataset_folder_name + dataset_count_filename);

	file << dataset_count;
}

int enn::LoadDatasetCount()
{
	int dataset_count = 0;

	std::ifstream file(dataset_folder_name + dataset_count_filename);
	if (file.fail())
		throw std::runtime_error("Failed to load file " + dataset_folder_name + dataset_count_filename);

	file >> dataset_count;

	return dataset_count;
}