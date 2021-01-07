#pragma once
#include "../math/vec3.h"
#include <vector>
#include <string>

namespace enn
{
	struct DatasetFrame
	{
		ema::vec3 camera_position;
		ema::vec3 camera_rotation;
		long long time;
	};

	struct DatasetVideo
	{
		// Specify scene (Will hopefully have multiple scenes)
		std::vector<DatasetFrame> frames;

		void SaveToFile(int file_nr);
		void LoadFromFile(int file_nr);
	};

	class DatasetVideoRecorder
	{
	public:
		DatasetVideoRecorder();

		void StartRecording(int frame_count);
		void CaptureFrame(const DatasetFrame& frame);

		bool IsReady();


	private:
		int dataset_count = 0;

		bool is_ready = true;
		int current_frame = 0;
		int max_frame_count = 0;
		DatasetVideo dataset_video;

	private:
		void saveDatasetCount();
	};

	int LoadDatasetCount();

}
