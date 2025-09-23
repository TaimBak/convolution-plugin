/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ImpulseResponseSelector.h"

//==============================================================================
/**
*/
class TDConvolveAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    TDConvolveAudioProcessor();
    ~TDConvolveAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
	void setCurrentIR(const ImpulseResponseSelector::IRItem* ir) { currentIR = ir; }

    //==============================================================================

    bool loadWavFile(const juce::File& file);
    bool exportProcessedWav(const juce::File& outFile, int blockSize = 480);

private:

    //==============================================================================
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> fileBuffer;   // holds the loaded audio
    double fileSampleRate = 0.0;
    bool fileLoaded = false;

    //==============================================================================
	const ImpulseResponseSelector::IRItem* currentIR = nullptr; // pointer to the current IR

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDConvolveAudioProcessor)
};
