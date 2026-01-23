/*
  ==============================================================================

    FreqDomainConvolver.h
    Created: 20 Nov 2025 10:41:43am
    Author:  Austin Clark

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <stdexcept>

class FreqDomainConvolver
{
public:
    // fftOrder=10 -> K=1024, blockSize=128 by your spec
    FreqDomainConvolver(const std::vector<float>& h, int fftOrder, int blockSize);

    void reset();

    // Process one block (numSamples can be < B for the final block)
    std::vector<float> processBlock(const float* x, int numSamples);

    // Convenience overload
    std::vector<float> processBlock(const std::vector<float>& x);

    // After feeding all input blocks, call this to collect the remaining N-1 samples.
    std::vector<float> flush();

    int getFFTSize()   const { return K; }
    int getBlockSize() const { return B; }
    int getIRLength()  const { return N; }

private:
    const int fftOrder;    // dsp::fft requirement
    const int K;           // FFT size
    const int B;           // processing block size
    const int N;           // IR length

    juce::dsp::FFT fft;

    // Frequency-domain working buffers (interleaved complex: 2*K floats)
    using C = juce::dsp::Complex<float>;
    std::vector<C> Hspec, Xspec, Yspec, timeC;

    // Time-domain working buffer (K real samples) and carry buffer (K)
    std::vector<float> timeK;
    std::vector<float> overlap;
};
