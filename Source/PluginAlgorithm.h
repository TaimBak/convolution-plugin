/*
  ==============================================================================

    PluginAlgorithm.h
    Created: 10 Sep 2025 2:22:18pm
    Author:  izanagi

  ==============================================================================
*/

#pragma once

#include <vector>
#include <stdexcept>

class TimeDomainConvolver {
public:
    TimeDomainConvolver(const std::vector<float>& ir);
    void reset();
	float processSample(float x);
    void processBlock(const float* in, float* out, std::size_t numSamples);

private:
	std::vector<float> ir;
    std::size_t        irSize;
    std::vector<float> delayBuffer;
    std::size_t        writeIndex;
};
