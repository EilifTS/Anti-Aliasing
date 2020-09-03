#pragma once
#include <string>
#include <sstream>
#include <iomanip>

namespace emisc
{
	template <typename T>
	inline std::string ToString(T v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	// Converts time in microseconds to string in hh:mm:ssssssss format
	inline std::string TimeToString(long long t)
	{
		double seconds;
		long long mins;
		long long hours;
		seconds = double(t % 60000000) / 1000000.0;
		mins = (t / 60000000) % 60;
		hours = (t / 3600000000);

		std::stringstream ss;
		ss << std::fixed << std::setprecision(6);
		ss << hours << "h " << mins << "m " << seconds << "s";
		return ss.str();
	}
}