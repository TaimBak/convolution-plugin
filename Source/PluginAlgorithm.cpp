/*
  ==============================================================================

    PluginAlgorithm.cpp
    Created: 10 Sep 2025 2:22:18pm
    Author:  izanagi

  ==============================================================================
*/

#include "PluginAlgorithm.h"


TimeDomainConvolver::TimeDomainConvolver(const std::vector<float>& inputIR)
    : ir(inputIR), irSize(ir.size()), delayBuffer(std::max<std::size_t>(1, ir.size()), 0.0f), writeIndex(0)
{
    if (irSize == 0) throw std::invalid_argument("IR cannot be empty");
}

void TimeDomainConvolver::reset()
{
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    writeIndex= 0;
};

float TimeDomainConvolver::processSample(float x)
{
    // Buffer the input sample
    delayBuffer[writeIndex] = x;

    // y[n] = sum_{k=0}^{irSize-1} ir[k] * inputBuffer[tempIndex]
    // Walk the IR forward and the delay line backward
    float sum = 0.0f;
    std::size_t tempIndex = writeIndex;
    for (std::size_t k = 0; k < irSize; k++)
    {
        sum += ir[k] * delayBuffer[tempIndex];
        if (tempIndex == 0)
            tempIndex = irSize - 1;
        else
            tempIndex--;
    }

    // Advance the write index
    writeIndex = (writeIndex + 1) % irSize;

    return sum;
}

void TimeDomainConvolver::processBlock(const float* in, float* out, std::size_t numSamples)
{
	std::vector<float> diagnostics(numSamples);

    for (std::size_t n = 0; n < numSamples; n++)
    {
        out[n] = processSample(in[n]);
        diagnostics[n] = out[n]; // For debugging purposes
    }

    return;
}