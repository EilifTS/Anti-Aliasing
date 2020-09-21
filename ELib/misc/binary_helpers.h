#pragma once

namespace emisc
{
	// Returns hamming weight of a number
	inline int HammingWeight(unsigned int b)
	{
		int out = 0;
		while (b)
		{
			out++;
			b &= (b - 1);
		}
		return out;
	}

	// Returns the bit length of a number
	// 10110 returns 5
	inline int BitLength(unsigned int b)
	{
		int out = 0;
		while (b)
		{
			out++;
			b >>= 1;
		}
		return out;
	}

	// Reverses the binary representation of a number
	// 10110 returns 01101
	inline unsigned int ReverseBinary(unsigned int b)
	{
		unsigned int out = 0;
		int bl = BitLength(b);
		for (int i = 0; i < bl; i++)
		{
			unsigned int shift = bl - i - 1;
			unsigned int bit = (b & (0x01 << shift)) >> shift;
			out |= bit << i;
		}
		return out;
	}

	// 10100 return 0.10100 in floating point
	inline float GetBinaryDecimal(unsigned int b)
	{
		float out = 0.0f;
		int bl = BitLength(b);
		float factor = 0.5f;
		for (int i = bl - 1; i >= 0; i--)
		{
			float bit = (b << i) == 0 ? 0.0f : 1.0f;
			out += factor * bit;
			factor *= 0.5f;
		}
		return out;
	}
}