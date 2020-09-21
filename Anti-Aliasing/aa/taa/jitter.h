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
	
	const ema::vec2& Get(int index) const { return points[index]; };
	int SampleCount() const { return (int)points.size(); };
private:
	std::vector<ema::vec2> points;
};