#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "PluginAlgorithm.h" // TimeDomainConvolver
#include "FreqDomainConvolver.h"

// --- Simple waveform view (renders from an AudioBuffer*) ----------------------
class WaveformView : public juce::Component
{
public:
    void setBuffer (const juce::AudioBuffer<float>* newBuffer, double newSampleRate)
    {
        buffer = newBuffer;
        sampleRate = newSampleRate;
        repaint();
    }

    void setTitle (juce::String newTitle)
    {
        title = std::move(newTitle);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.6f));
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRect (getLocalBounds());

        auto r = getLocalBounds().reduced (8, 8);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawFittedText (title.isEmpty() ? "Waveform" : title, r.removeFromTop(18), juce::Justification::centredLeft, 1);

        // Midline
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine ((float) r.getX(), (float) r.getCentreY(), (float) r.getRight(), (float) r.getCentreY());

        if (buffer == nullptr || buffer->getNumSamples() == 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawFittedText ("(no audio)", r, juce::Justification::centred, 1);
            return;
        }

        // Draw a min/max envelope per x pixel (mono-averaged if multiple channels)
        g.reduceClipRegion (r);
        const int width  = r.getWidth();
        const int height = r.getHeight();
        const int N      = buffer->getNumSamples();
        const int C      = buffer->getNumChannels();
        if (width <= 1 || height <= 1) return;

        // samples represented by each horizontal pixel
        const int spp = juce::jmax (1, N / width);

        const int x0 = r.getX();
        const int y0 = r.getY();
        const float midY = (float) (y0 + height * 0.5f);
        const float halfH = (float) (height * 0.5f);

        juce::Colour waveColour = juce::Colours::aqua.brighter (0.1f).withAlpha (0.9f);
        g.setColour (waveColour);

        for (int px = 0; px < width; ++px)
        {
            const int start = px * spp;
            const int end   = juce::jmin (start + spp, N);

            float vmin =  1.0f;
            float vmax = -1.0f;

            for (int i = start; i < end; ++i)
            {
                // average all channels for display
                float s = 0.0f;
                for (int ch = 0; ch < C; ++ch)
                    s += buffer->getReadPointer (ch)[i];
                s /= (float) C;

                vmin = juce::jmin (vmin, s);
                vmax = juce::jmax (vmax, s);
            }

            // Map [-1,1] -> vertical pixels (inverted Y)
            const float yTop = juce::jlimit (0.0f, (float) getHeight(),
                                             midY - vmax * halfH);
            const float yBot = juce::jlimit (0.0f, (float) getHeight(),
                                             midY - vmin * halfH);

            g.drawVerticalLine (x0 + px, yTop, yBot);
        }
    }

private:
    const juce::AudioBuffer<float>* buffer = nullptr; // not owned
    double sampleRate = 0.0;
    juce::String title;
};


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

    juce::GroupComponent convGroup { "Convolution", "Time-Domain Convolution Settings" };

    // New: Dry/Wet mix slider
    juce::Slider mixSlider;
    juce::Label  mixLabel { {}, "Dry/Wet" };

    juce::ToggleButton normalizeToggle { "Normalize to -1 dBFS" };

    // Progress display
    std::atomic<double> progressAtomic { 0.0 };
    double              progressValue  = 0.0;
    juce::ProgressBar   progressBar    { progressValue };

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
    void processOfflineConvolutionTD();
    void processOfflineConvolutionFD();
    void exportWavAsync();

    // ===== Helpers =====
    void setStatus (const juce::String& msg);
    void setAudioInfo (const juce::String& msg);
    void setIRInfo (const juce::String& msg);
    void updateEnables();
    void normalizeToDbFS (float targetDb);

    // Timer: mirror atomic progress -> double for ProgressBar
    void timerCallback() override;
    
    // Graphs
    WaveformView waveformIn;
    WaveformView waveformProcessed;
    WaveformView waveformIR;
    
    // Keep the original audio separate for the "Input" graph
    std::unique_ptr<juce::AudioBuffer<float>> originalBuffer;

    // Buffer to visualize the IR (mono)
    juce::AudioBuffer<float> irDisplayBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
