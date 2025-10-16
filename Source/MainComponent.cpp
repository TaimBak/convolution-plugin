#include "MainComponent.h"

//==============================================================================
// Constructor - sets up UI
MainComponent::MainComponent()
{
    // Buttons
    addAndMakeVisible (loadAudioButton);
    addAndMakeVisible (loadIRButton);
    addAndMakeVisible (processButton);
    addAndMakeVisible (exportButton);

    loadAudioButton.onClick  = [this] { loadAudioAsync(); };
    loadIRButton.onClick     = [this] { loadIRAsync();    };
    processButton.onClick    = [this] { processOfflineConvolution(); };
    exportButton.onClick     = [this] { exportWavAsync(); };

    // Labels
    audioInfoLabel.setJustificationType (juce::Justification::centredLeft);
    irInfoLabel   .setJustificationType (juce::Justification::centredLeft);
    statusLabel   .setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (audioInfoLabel);
    addAndMakeVisible (irInfoLabel);
    addAndMakeVisible (statusLabel);

    // Convolution group
    addAndMakeVisible (convGroup);
    addAndMakeVisible (normalizeToggle);
    normalizeToggle.setToggleState (false, juce::dontSendNotification);

    processButton.setEnabled (false);
    exportButton.setEnabled (false);
    
    addAndMakeVisible(progressBar);
    progressBar.setPercentageDisplay(true);
    startTimerHz(20); // ~20 fps UI update is plenty

    setSize (920, 420);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.withBrightness (0.12f));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (12);

    // Top row buttons
    auto top = area.removeFromTop (36);
    loadAudioButton.setBounds (top.removeFromLeft (200).reduced (0, 2));
    top.removeFromLeft (8);
    loadIRButton.setBounds (top.removeFromLeft (200).reduced (0, 2));
    top.removeFromLeft (8);
    processButton.setBounds (top.removeFromLeft (220).reduced (0, 2));
    top.removeFromLeft (8);
    exportButton.setBounds (top.removeFromLeft (200).reduced (0, 2));

    area.removeFromTop (8);

    // Info rows
    auto info1 = area.removeFromTop (24);
    audioInfoLabel.setBounds (info1.removeFromLeft (area.getWidth()).reduced (2, 0));
    area.removeFromTop (6);
    auto info2 = area.removeFromTop (24);
    irInfoLabel.setBounds (info2.removeFromLeft (area.getWidth()).reduced (2, 0));
    area.removeFromTop (6);
    auto info3 = area.removeFromTop (24);
    statusLabel.setBounds (info3.removeFromLeft (area.getWidth()).reduced (2, 0));

    area.removeFromTop (10);

    // Convolution group
    convGroup.setBounds (area);
    auto convArea = convGroup.getBounds().reduced (14);
    normalizeToggle.setBounds (convArea.removeFromTop (32).removeFromLeft (300));
    
    auto bottom = getLocalBounds().reduced(12).removeFromBottom(24);
    progressBar.setBounds(bottom);
}

//==============================================================================
// Async: Load main audio WAV
void MainComponent::loadAudioAsync()
{
    openAudioChooser = std::make_unique<juce::FileChooser>(
        "Select an audio WAV to process",
        juce::File(),
        "*.wav",
        true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectFiles;

    openAudioChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (! file.existsAsFile())
        {
            setStatus ("Audio load cancelled.");
            openAudioChooser.reset();
            return;
        }

        juce::AudioFormatManager afm;
        afm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (afm.createReaderFor (file));

        if (! reader)
        {
            setStatus ("Could not open audio file.");
            openAudioChooser.reset();
            return;
        }

        const int numSamples  = static_cast<int> (reader->lengthInSamples);
        const int numChannels = static_cast<int> (reader->numChannels);

        auto newBuffer = std::make_unique<juce::AudioBuffer<float>>(numChannels, numSamples);
        if (! reader->read (newBuffer.get(), 0, numSamples, 0, true, true))
        {
            setStatus ("Failed to read audio samples.");
            openAudioChooser.reset();
            return;
        }

        audioBuffer = std::move (newBuffer);
        sampleRateHz = reader->sampleRate;
        originalNumChannels = numChannels;

        setAudioInfo (juce::String::formatted(
            "Audio: %s  |  %d ch, %.0f Hz, %d samples",
            file.getFileName().toRawUTF8(),
            numChannels,
            sampleRateHz,
            numSamples));

        setStatus ("Audio loaded.");
        openAudioChooser.reset();
        updateEnables();
    });
}

//==============================================================================
// Async: Load IR WAV -> mono vector<float>
void MainComponent::loadIRAsync()
{
    openIRChooser = std::make_unique<juce::FileChooser>(
        "Select an IR WAV (will be used in mono)",
        juce::File(),
        "*.wav",
        true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectFiles;

    openIRChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (! file.existsAsFile())
        {
            setStatus ("IR load cancelled.");
            openIRChooser.reset();
            return;
        }

        juce::AudioFormatManager afm;
        afm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (afm.createReaderFor (file));

        if (! reader)
        {
            setStatus ("Could not open IR file.");
            openIRChooser.reset();
            return;
        }

        const int irSamples  = static_cast<int> (reader->lengthInSamples);
        const int irCh       = static_cast<int> (reader->numChannels);

        juce::AudioBuffer<float> irBuf (irCh, irSamples);
        if (! reader->read (&irBuf, 0, irSamples, 0, true, true))
        {
            setStatus ("Failed to read IR samples.");
            openIRChooser.reset();
            return;
        }

        std::vector<float> monoIR;
        monoIR.resize ((size_t) irSamples, 0.0f);

        // Average to mono if needed
        for (int n = 0; n < irSamples; ++n)
        {
            double acc = 0.0;
            for (int c = 0; c < irCh; ++c)
                acc += irBuf.getReadPointer (c)[n];
            monoIR[(size_t) n] = static_cast<float> (acc / juce::jmax (1, irCh));
        }

        impulseResponse = std::move (monoIR);
        irSampleRateHz = reader->sampleRate;

        juce::String srNote;
        if (audioBuffer && std::abs (irSampleRateHz - sampleRateHz) > 1.0)
            srNote = juce::String::formatted("  (SR mismatch: IR %.0f Hz, Audio %.0f Hz)", irSampleRateHz, sampleRateHz);

        setIRInfo (juce::String::formatted(
            "IR: %s  |  mono, %.0f Hz, %d taps%s",
            file.getFileName().toRawUTF8(),
            irSampleRateHz,
            irSamples,
            srNote.toRawUTF8()));

        setStatus ("IR loaded.");
        openIRChooser.reset();
        updateEnables();
    });
}

//==============================================================================
// Offline convolution using TimeDomainConvolver per channel
void MainComponent::processOfflineConvolution()
{
    if (audioBuffer == nullptr)              { setStatus("Load an audio WAV first."); return; }
    if (impulseResponse.empty())             { setStatus("Load an IR WAV first.");   return; }

    setStatus("Processing...");
    processButton.setEnabled(false);
    exportButton.setEnabled(false);
    progressAtomic.store(0.0, std::memory_order_relaxed);

    const int numCh   = audioBuffer->getNumChannels();
    const int numSmps = audioBuffer->getNumSamples();

    auto in  = std::make_unique<juce::AudioBuffer<float>>(*audioBuffer);
    auto out = std::make_unique<juce::AudioBuffer<float>>(numCh, numSmps);
    auto ir  = impulseResponse; // copy for worker

    auto safe = juce::Component::SafePointer<MainComponent>(this);

    std::thread([safe, in = std::move(in), out = std::move(out), ir = std::move(ir), numCh, numSmps]() mutable
    {
        juce::ScopedNoDenormals noDenormals;
        juce::FloatVectorOperations::disableDenormalisedNumberSupport();

        // Tune chunk size (samples) for responsiveness
        constexpr int CHUNK = 16384; // ~0.37 s @ 44.1 kHz per chunk
        const double totalWork = static_cast<double>(numCh) * static_cast<double>(numSmps);

        for (int ch = 0; ch < numCh; ++ch)
        {
            TimeDomainConvolver conv(ir);
            conv.reset();

            int done = 0;
            while (done < numSmps)
            {
                const int block = std::min(CHUNK, numSmps - done);

                conv.processBlock(in->getReadPointer(ch)  + done,
                                  out->getWritePointer(ch) + done,
                                  static_cast<std::size_t>(block));

                done += block;

                if (auto* self = safe.getComponent())
                {
                    // ---- this is the value you must STORE (replace "..." with this) ----
                    const double fraction = (static_cast<double>(ch) * numSmps + done) / totalWork;
                    self->progressAtomic.store(fraction, std::memory_order_relaxed);
                    // --------------------------------------------------------------------
                }
                else
                {
                    return; // UI destroyed—stop work
                }
            }
        }

        // Hand off result to UI thread (no unique_ptr captured by callAsync)
        auto* outRaw = out.release();
        juce::MessageManager::callAsync([safe, outRaw]()
        {
            if (auto* self = safe.getComponent())
            {
                self->audioBuffer.reset(outRaw);

                if (self->normalizeToggle.getToggleState())
                    self->normalizeToDbFS(-1.0f);

                self->progressAtomic.store(1.0, std::memory_order_relaxed);
                self->setStatus("Processing complete.");
                self->updateEnables();
            }
            else
            {
                delete outRaw; // UI vanished
            }
        });

    }).detach();
}


//==============================================================================
// Async export as 24-bit WAV
void MainComponent::exportWavAsync()
{
    if (audioBuffer == nullptr)
    {
        setStatus ("Nothing to export—process audio first.");
        return;
    }

    saveChooser = std::make_unique<juce::FileChooser>(
        "Export processed WAV",
        juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("processed.wav"),
        "*.wav",
        true);

    auto flags = juce::FileBrowserComponent::saveMode
               | juce::FileBrowserComponent::canSelectFiles;

    saveChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
    {
        auto outFile = fc.getResult();
        if (outFile == juce::File())
        {
            setStatus ("Export cancelled.");
            saveChooser.reset();
            return;
        }

        if (outFile.existsAsFile())
            outFile.deleteFile();

        std::unique_ptr<juce::FileOutputStream> stream (outFile.createOutputStream());
        if (! stream || ! stream->openedOk())
        {
            setStatus ("Couldn't open output file for writing.");
            saveChooser.reset();
            return;
        }

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (stream.release(),
                                 sampleRateHz,
                                 (unsigned int) audioBuffer->getNumChannels(),
                                 24, // 24-bit WAV
                                 {}, 0));

        if (! writer)
        {
            setStatus ("Failed to create WAV writer.");
            saveChooser.reset();
            return;
        }

        const bool ok = writer->writeFromAudioSampleBuffer (*audioBuffer, 0, audioBuffer->getNumSamples());
        setStatus (ok ? ("Exported: " + outFile.getFullPathName())
                      : "Write error.");

        saveChooser.reset();
    });
}

//==============================================================================
// Helpers

void MainComponent::setStatus (const juce::String& msg)
{
    statusLabel.setText (msg, juce::dontSendNotification);
}

void MainComponent::setAudioInfo (const juce::String& msg)
{
    audioInfoLabel.setText (msg, juce::dontSendNotification);
}

void MainComponent::setIRInfo (const juce::String& msg)
{
    irInfoLabel.setText (msg, juce::dontSendNotification);
}

void MainComponent::updateEnables()
{
    const bool hasAudio = (audioBuffer != nullptr);
    const bool hasIR    = (! impulseResponse.empty());

    processButton.setEnabled (hasAudio && hasIR);
    exportButton.setEnabled  (hasAudio); // enabled after load; still useful after processing
}

void MainComponent::normalizeToDbFS (float targetDb)
{
    const float targetLinear = juce::Decibels::decibelsToGain (targetDb);
    float peak = 0.0f;

    for (int ch = 0; ch < audioBuffer->getNumChannels(); ++ch)
        peak = std::max (peak, audioBuffer->getMagnitude (ch, 0, audioBuffer->getNumSamples()));

    if (peak <= 0.0f) return;

    const float scale = targetLinear / peak;
    for (int ch = 0; ch < audioBuffer->getNumChannels(); ++ch)
        audioBuffer->applyGain (ch, 0, audioBuffer->getNumSamples(), scale);
}

void MainComponent::timerCallback()
{
    // Mirror the atomic to the value the ProgressBar watches (message thread)
    const double p = progressAtomic.load(std::memory_order_relaxed);
    if (p != progressValue)
        progressValue = p; // ProgressBar reads this by reference
}
