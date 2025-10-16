#pragma once
#include <JuceHeader.h>
#include "PluginAlgorithm.h" // TimeDomainConvolver

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // ===== UI =====
    juce::TextButton loadAudioButton { "Load Audio WAV..." };
    juce::TextButton loadIRButton    { "Load IR WAV..."    };
    juce::TextButton processButton   { "Process Convolution" };
    juce::TextButton exportButton    { "Export WAV..."     };

    juce::Label audioInfoLabel { {}, "Audio: (none)" };
    juce::Label irInfoLabel    { {}, "IR: (none)"    };
    juce::Label statusLabel    { {}, "Ready."        };

    juce::GroupComponent convGroup { "Convolution", "Time-Domain Convolution (offline)" };
    juce::ToggleButton normalizeToggle { "Normalize to -1 dBFS after processing" };

    // ===== Audio data =====
    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer; // loaded/processed audio
    double sampleRateHz = 44100.0;
    int    originalNumChannels = 0;

    // ===== IR data (mono) =====
    std::vector<float> impulseResponse; // mono IR used for all channels
    double irSampleRateHz = 44100.0;

    // ===== File choosers must persist during async ops =====
    std::unique_ptr<juce::FileChooser> openAudioChooser;
    std::unique_ptr<juce::FileChooser> openIRChooser;
    std::unique_ptr<juce::FileChooser> saveChooser;

    // ===== Actions =====
    void loadAudioAsync();
    void loadIRAsync();
    void processOfflineConvolution();
    void exportWavAsync();

    // ===== Helpers =====
    void setStatus (const juce::String& msg);
    void setAudioInfo (const juce::String& msg);
    void setIRInfo (const juce::String& msg);
    void updateEnables();

    void normalizeToDbFS (float targetDb);
    
    // Progress tracking (atomic for worker thread -> double for ProgressBar)
    std::atomic<double> progressAtomic { 0.0 };
    double              progressValue  = 0.0;
    juce::ProgressBar   progressBar    { progressValue }; // ProgressBar needs a double&

    // Timer to mirror atomic -> double on the message thread
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
