/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TDConvolveAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    TDConvolveAudioProcessorEditor (TDConvolveAudioProcessor&);
    ~TDConvolveAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    TDConvolveAudioProcessor& audioProcessor;

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::TextButton loadBtn{ "Load WAV..." };
    juce::TextButton exportBtn{ "Export Processed..." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDConvolveAudioProcessorEditor)
};
