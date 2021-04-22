#pragma once
#include "math/vec2.h"
#include <vector>
#include "misc/sequences.h"

class Jitter
{
public:
	Jitter(int count) : points(count) {};

	static Jitter Halton(int base1, int base2, int count)
	{
		auto seq1 = emisc::HaltonSequence(base1, count);
		auto seq2 = emisc::HaltonSequence(base2, count);
		Jitter out(count);
		for (int i = 0; i < count; i++)
		{
			out.points[i] = ema::vec2(seq1[i], seq2[i]);
		}
		return out;
	}

	static Jitter Custom(int factor)
	{
		if (factor == 4)
		{
			Jitter out(16);
			out.points[0] = ema::vec2(2.0f, 1.0f);
			out.points[1] = ema::vec2(0.0f, 2.0f);
			out.points[2] = ema::vec2(2.0f, 3.0f);
			out.points[3] = ema::vec2(0.0f, 0.0f);
			out.points[4] = ema::vec2(1.0f, 2.0f);
			out.points[5] = ema::vec2(3.0f, 3.0f);
			out.points[6] = ema::vec2(1.0f, 0.0f);
			out.points[7] = ema::vec2(3.0f, 1.0f);
			out.points[8] = ema::vec2(0.0f, 3.0f);
			out.points[9] = ema::vec2(2.0f, 0.0f);
			out.points[10] = ema::vec2(0.0f, 1.0f);
			out.points[11] = ema::vec2(2.0f, 2.0f);
			out.points[12] = ema::vec2(3.0f, 0.0f);
			out.points[13] = ema::vec2(1.0f, 1.0f);
			out.points[14] = ema::vec2(3.0f, 2.0f);
			out.points[15] = ema::vec2(1.0f, 3.0f);

			for (int i = 0; i < (int)out.points.size(); i++)
				out.points[i] = 0.25f * (out.points[i] + ema::vec2(0.5f, 0.5f));
			return out;
		}
		else if (factor == 2)
		{
			auto seq1 = emisc::HaltonSequence(2, 5);
			auto seq2 = emisc::HaltonSequence(3, 5);
			std::vector<ema::vec2> halton_points =
			{
				{seq1[1], seq2[1]},
				{seq1[2], seq2[2]},
				{seq1[3], seq2[3]},
				{seq1[4], seq2[4]},
			};
			Jitter out(16);

			out.points[0] = ema::vec2(0.0f, 0.0f) + halton_points[0];
			out.points[1] = ema::vec2(1.0f, 1.0f) + halton_points[1];
			out.points[2] = ema::vec2(0.0f, 1.0f) + halton_points[2];
			out.points[3] = ema::vec2(1.0f, 0.0f) + halton_points[3];
			out.points[4] = ema::vec2(0.0f, 0.0f) + halton_points[1];
			out.points[5] = ema::vec2(1.0f, 1.0f) + halton_points[2];
			out.points[6] = ema::vec2(0.0f, 1.0f) + halton_points[3];
			out.points[7] = ema::vec2(1.0f, 0.0f) + halton_points[0];
			out.points[8] = ema::vec2(0.0f, 0.0f) + halton_points[2];
			out.points[9] = ema::vec2(1.0f, 1.0f) + halton_points[3];
			out.points[10] = ema::vec2(0.0f, 1.0f) + halton_points[0];
			out.points[11] = ema::vec2(1.0f, 0.0f) + halton_points[1];
			out.points[12] = ema::vec2(0.0f, 0.0f) + halton_points[3];
			out.points[13] = ema::vec2(1.0f, 1.0f) + halton_points[0];
			out.points[14] = ema::vec2(0.0f, 1.0f) + halton_points[1];
			out.points[15] = ema::vec2(1.0f, 0.0f) + halton_points[2]; 
			for (int i = 0; i < (int)out.points.size(); i++)
				out.points[i] = 0.5f * out.points[i];
			return out;
		}
		
	}
	
	const ema::vec2& Get(int index) const { return points[index]; };
	int SampleCount() const { return (int)points.size(); };
private:
	std::vector<ema::vec2> points;
};