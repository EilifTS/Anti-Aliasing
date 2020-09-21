#pragma once
#include <vector>
#include <assert.h>

namespace emisc
{
	inline std::vector<float> HaltonSequence(int base, int count)
	{
		assert(count != 0 && base != 0 && base != 1);
		std::vector<double> temp((long long)count + 1);
		temp[0] = 0.0;

		int offsets = base - 1;

		int i_start = 1;
		int i = 0;
		int nr = 1;
		double factor = 1.0 / (double)base;
		while (nr <= count)
		{
			for (int offset = 1; offset <= offsets; offset++)
			{
				for (int i = 0; i < i_start; i++)
				{
					temp[nr] = temp[i] + (double)offset * factor;
					nr++;
					if (nr == count + 1)
					{
						std::vector<float> out(count);
						for (int i = 0; i < count; i++)
						{
							out[i] = (float)temp[(long long)i + 1];
						}

						return out;
					}
				}
			}
		
			i_start += i_start * offsets;
			factor *= 1.0 / (double)base;
		}

		assert(0);
		return std::vector<float>();
	}
}