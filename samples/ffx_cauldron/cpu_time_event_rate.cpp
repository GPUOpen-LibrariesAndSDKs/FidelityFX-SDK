// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "cpu_time_event_rate.h"
#include <numeric>
#include <windows.h>

CpuTimeEventRate::CpuTimeEventRate()
{
	constexpr uint32_t measurementCount = 32;
	data.resize(measurementCount);
}

double CpuTimeEventRate::MillisecondsNow()
{
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc;
	s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	double milliseconds = 0;

	if (s_use_qpc)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		milliseconds = double(1000.0 * now.QuadPart) / s_frequency.QuadPart;
	}
	else
	{
		milliseconds = double(GetTickCount());
	}

	return milliseconds;
}

void CpuTimeEventRate::Update()
{
	double timeNow = MillisecondsNow();
	double deltaTime = timeNow - lastEventTime;
	lastEventTime = timeNow;

	data[currentIndex] = (float)deltaTime;
	currentIndex = (currentIndex + 1) % data.size();

	// get avg value
	{
		float sum = std::accumulate(data.begin(), data.end(), 0.0f);
		uint32_t zeroCount = (uint32_t)std::count(data.begin(), data.end(), 0.0f);
		float deltaTimeAvg = sum / std::max(uint32_t(data.size() - zeroCount), uint32_t(1));
		avgFps = (uint32_t)(1e3f / std::max(deltaTimeAvg, float(1e-32)));
	}
	// get last value
	fps = (uint32_t)(1e3f / std::max(deltaTime, double(1e-32)));
}

uint32_t CpuTimeEventRate::getRate()
{
	return fps;
}

uint32_t CpuTimeEventRate::getStableRate()
{
	double timeNow = MillisecondsNow();
	if (timeNow - lastGetTime > 1e3)
	{
		stableFps = avgFps;
		lastGetTime = timeNow;
	}
	return stableFps;
}

