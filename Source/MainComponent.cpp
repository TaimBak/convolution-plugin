#include "MainComponent.h"
#include <chrono>

//==============================================================================
// Constructor - sets up UI
MainComponent::MainComponent()
{
    // Buttons
    addAndMakeVisible (loadAudioButton);
    addAndMakeVisible (loadIRButton);
    addAndMakeVisible (processButton);
    addAndMakeVisible (exportButton);

    loadAudioButton.onClick = [this] { loadAudioAsync(); };
    loadIRButton.onClick = [this] { loadIRAsync();    };
    processButton.onClick = [this] { processOfflineConvolutionFD(); };
    exportButton.onClick = [this] { exportWavAsync(); };

    // Labels
    audioInfoLabel.setJustificationType (juce::Justification::centredLeft);
    irInfoLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (audioInfoLabel);
    addAndMakeVisible (irInfoLabel);
    addAndMakeVisible (statusLabel);

    // Convolution group
    addAndMakeVisible (convGroup);

    // --- Dry/Wet slider setup ---
    mixSlider.setRange (0.0, 100.0, 1.0);
    mixSlider.setTextValueSuffix (" %");
    mixSlider.setTooltip ("0% = fully dry, 100% = fully wet");
    mixSlider.setValue (50.0); // default 50/50
    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    addAndMakeVisible (mixSlider);

    mixLabel.attachToComponent (&mixSlider, true); // left-side label
    addAndMakeVisible (mixLabel);

    addAndMakeVisible (normalizeToggle);
    normalizeToggle.setToggleState (false, juce::dontSendNotification);

    // Progress bar
    addAndMakeVisible(progressBar);
    progressBar.setPercentageDisplay(true);
    startTimerHz(20); // UI refresh for progress (20 fps is plenty)
    
    addAndMakeVisible (waveformIn);
    addAndMakeVisible (waveformProcessed);
    addAndMakeVisible (waveformIR);

    waveformIn.setTitle("Input");
    waveformProcessed.setTitle("Processed");
    waveformIR.setTitle("Impulse Response");


    processButton.setEnabled (false);
    exportButton.setEnabled (false);

    setSize (920, 720);
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
    loadIRButton.setBounds (top.removeFromLeft (200).reduced (0, 2));
    top.removeFromLeft (8);
    loadAudioButton.setBounds (top.removeFromLeft (200).reduced (0, 2));
    top.removeFromLeft (8);
    processButton.setBounds (top.removeFromLeft (220).reduced (0, 2));
    top.removeFromLeft (8);
    exportButton.setBounds (top.removeFromLeft (200).reduced (0, 2));

    area.removeFromTop (8);

    // Info rows
    auto info1 = area.removeFromTop (24);
    irInfoLabel.setBounds (info1.removeFromLeft (area.getWidth()).reduced (2, 0));
    area.removeFromTop (6);
    auto info2 = area.removeFromTop (24);
    audioInfoLabel.setBounds (info2.removeFromLeft (area.getWidth()).reduced (2, 0));
    area.removeFromTop (6);
    auto info3 = area.removeFromTop (24);
    statusLabel.setBounds (info3.removeFromLeft (area.getWidth()).reduced (2, 0));

    area.removeFromTop (5);
    
    auto progress = area.removeFromTop (24);
    progressBar.setBounds (progress);
    
    area.removeFromTop(10);
    
    // Convolution group area
    convGroup.setBounds (area.removeFromTop (65));
    auto convArea = convGroup.getBounds().reduced (14);

    // Layout: [Label][Slider]   [Normalize toggle]
    auto row = convArea.removeFromTop (32);
    auto labelW = 90;
    mixLabel.setBounds (row.removeFromLeft (labelW));
    mixSlider.setBounds (row.removeFromLeft (convArea.getWidth() - labelW - 220));
    row.removeFromLeft (12);
    normalizeToggle.setBounds (row.removeFromLeft (220));
    
    area.removeFromTop (8);

    const int gap  = 4;
    const int graphsMinHeight = 360;
    auto graphsArea = area;
    if (graphsArea.getHeight() < graphsMinHeight)
        graphsArea.setHeight (graphsMinHeight);

    // Stack 3 equal-height rows
    const int rowH = (graphsArea.getHeight() - 2 * gap) / 3;

    row = graphsArea.removeFromTop (rowH);
    waveformIR.setBounds (row);

    graphsArea.removeFromTop (gap);
    row = graphsArea.removeFromTop (rowH);
    waveformIn.setBounds (row);

    graphsArea.removeFromTop (gap);
    row = graphsArea.removeFromTop (rowH);
    waveformProcessed.setBounds (row);
}

//==============================================================================
void MainComponent::timerCallback()
{
    const double p = progressAtomic.load(std::memory_order_relaxed);
    if (p != progressValue)
        progressValue = p;
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
            file.getFileName().toRawUTF8(), numChannels, sampleRateHz, numSamples));

        setStatus ("Audio loaded.");
        openAudioChooser.reset();
        updateEnables();
        originalBuffer = std::make_unique<juce::AudioBuffer<float>>(*audioBuffer);
        waveformIn.setBuffer(originalBuffer.get(), sampleRateHz);
        waveformProcessed.setBuffer (nullptr, 0.0);
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

        std::vector<float> monoIR ((size_t) irSamples, 0.0f);

        // Average to mono if needed
        for (int n = 0; n < irSamples; ++n)
        {
            double acc = 0.0;
            for (int c = 0; c < irCh; ++c)
                acc += irBuf.getReadPointer (c)[n];
            monoIR[(size_t) n] = static_cast<float> (acc / juce::jmax (1, irCh));
        }
        
//        // ===== DEBUG: Print IR statistics =====
//        double absSum = 0.0;
//        double energySum = 0.0;
//        float peakVal = 0.0f;
//
//        for (size_t i = 0; i < monoIR.size(); ++i)
//        {
//            float s = monoIR[i];
//            absSum += std::abs(s);
//            energySum += s * s;
//            peakVal = std::max(peakVal, std::abs(s));
//        }
//
//        DBG("IR samples: " << monoIR.size());
//        DBG("IR peak value: " << peakVal);
//        DBG("IR sum(|h|): " << absSum);
//        DBG("IR sqrt(sum(h²)): " << std::sqrt(energySum));
//        // ======================================
//        
//        // Normalize for approximately unity gain through convolution
//        double energySum = 0.0;
//        for (size_t i = 0; i < monoIR.size(); ++i)
//            energySum += static_cast<double>(monoIR[i]) * static_cast<double>(monoIR[i]);
//
//        if (energySum > 0.0)
//        {
//            // Scale target based on IR length relative to a "reference" length
//            const double refLength = 1024.0;  // Reference IR length
//            const double lengthFactor = std::sqrt(monoIR.size() / refLength);
//            const double target = 0.7 * lengthFactor;
//            
//            const float compensation = static_cast<float>(target / std::sqrt(energySum));
//            
//            for (size_t i = 0; i < monoIR.size(); ++i)
//                monoIR[i] *= compensation;
//                
//            DBG("IR compensation factor: " << compensation);
//        }

        impulseResponse = std::move (monoIR);
        irSampleRateHz = reader->sampleRate;

        juce::String srNote;
        if (audioBuffer && std::abs (irSampleRateHz - sampleRateHz) > 1.0)
            srNote = juce::String::formatted("  (SR mismatch: IR %.0f Hz, Audio %.0f Hz)", irSampleRateHz, sampleRateHz);

        setIRInfo (juce::String::formatted(
            "IR: %s  |  1 ch, %.0f Hz, %d taps%s",
            file.getFileName().toRawUTF8(), irSampleRateHz, irSamples, srNote.toRawUTF8()));

        setStatus ("IR loaded.");
        openIRChooser.reset();
        
        // Build a tiny 1-channel buffer that mirrors the IR vector for display
        irDisplayBuffer.setSize (1, (int) impulseResponse.size());
        if (! impulseResponse.empty())
            irDisplayBuffer.copyFrom (0, 0, impulseResponse.data(), (int) impulseResponse.size());

        waveformIR.setBuffer (&irDisplayBuffer, irSampleRateHz);

        
        updateEnables();
    });
}

//==============================================================================
// Offline convolution using processSample() in the time domain
void MainComponent::processOfflineConvolutionTD()
{
    if (audioBuffer == nullptr)  { setStatus ("Load an audio WAV first."); return; }
    if (impulseResponse.empty()) { setStatus ("Load an IR WAV first.");   return; }

    setStatus ("Processing...");
    processButton.setEnabled(false);
    exportButton.setEnabled(false);
    progressAtomic.store(0.0, std::memory_order_relaxed);

    const int numCh   = audioBuffer->getNumChannels();
    const int numSmps = audioBuffer->getNumSamples();

    // Capture dry/wet mix from the UI (0..1)
    const float wetMix = juce::jlimit (0.0f, 1.0f, (float) (mixSlider.getValue() / 100.0));
    const float dryMix = 1.0f - wetMix;

    auto in  = std::make_unique<juce::AudioBuffer<float>>(*audioBuffer);
    auto out = std::make_unique<juce::AudioBuffer<float>>(numCh, numSmps);
    auto ir  = impulseResponse; // copy for worker

    auto safe = juce::Component::SafePointer<MainComponent>(this);

    std::thread([safe, in = std::move(in), out = std::move(out), ir = std::move(ir),
                 numCh, numSmps, dryMix, wetMix]() mutable
    {
        juce::ScopedNoDenormals noDenormals;
        juce::FloatVectorOperations::disableDenormalisedNumberSupport();
        
        const auto start = std::chrono::steady_clock::now();

        constexpr int CHUNK = 1 << 15; // 32768 samples per chunk
        const double totalWork = static_cast<double>(numCh) * static_cast<double>(numSmps);

        for (int ch = 0; ch < numCh; ++ch)
        {
            TimeDomainConvolver conv(ir);
            conv.reset();

            const float* inPtr  = in->getReadPointer(ch);
            float*       outPtr = out->getWritePointer(ch);

            int done = 0;
            while (done < numSmps)
            {
                const int block = std::min(CHUNK, numSmps - done);

                // Per-sample to control mixing
                for (int i = 0; i < block; ++i)
                {
                    const float x = inPtr[done + i];               // dry input
                    const float yWet = conv.processSample(x);     // wet
                    outPtr[done + i] = (dryMix * x) + (wetMix * yWet); // linear
                }

                done += block;

                if (auto* self = safe.getComponent())
                {
                    const double fraction = (static_cast<double>(ch) * numSmps + done) / totalWork;
                    self->progressAtomic.store(fraction, std::memory_order_relaxed);
                }
                else
                {
                    return; // UI destroyed
                }
            }
        }
        
        const double elapsedSec =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        juce::String durationText = (elapsedSec < 1.0)
            ? juce::String::formatted("%.0f ms", elapsedSec * 1000.0)
            : juce::String(elapsedSec, 2) + " s";

        auto outOwned = std::move(out); // outOwned is a std::unique_ptr<AudioBuffer<float>>

        juce::MessageManager::callAsync(
            [safe, outOwned = std::move(outOwned), durationText = std::move(durationText)]() mutable
            {
                if (auto* self = safe.getComponent())
                {
                    // Transfer ownership atomically to your member unique_ptr
                    self->audioBuffer = std::move(outOwned);

                    self->waveformProcessed.setBuffer (self->audioBuffer.get(), self->sampleRateHz);
                    
                    if (self->normalizeToggle.getToggleState())
                        self->normalizeToDbFS(-1.0f);

                    self->progressAtomic.store(1.0, std::memory_order_relaxed);
                    self->setStatus("Processing complete (" + durationText + ").");
                    self->updateEnables();
                }
                // else: the moved unique_ptr just goes out of scope here; auto-freed safely
            });

    }).detach();
}

// Offline convolution using frequency-domain OLA
void MainComponent::processOfflineConvolutionFD()
{
    // No work if no files
    if (audioBuffer == nullptr)  { setStatus ("Load an audio WAV first."); return; }
    if (impulseResponse.empty()) { setStatus ("Load an IR WAV first.");   return; }

    // Progress Bar trigger
    setStatus ("Processing...");
    processButton.setEnabled(false);
    exportButton.setEnabled(false);
    progressAtomic.store(0.0, std::memory_order_relaxed);

    const int numCh   = audioBuffer->getNumChannels();
    const int numSmps = audioBuffer->getNumSamples();

    // Wet/Dry (0..1)
    const float wetMix = juce::jlimit (0.0f, 1.0f, (float) (mixSlider.getValue() / 100.0));
    const float dryMix = 1.0f - wetMix;
    const float wetGain = 90.0;  // ≈ 2048

    auto in  = std::make_unique<juce::AudioBuffer<float>>(*audioBuffer);
    auto out = std::make_unique<juce::AudioBuffer<float>>(numCh, numSmps); // temp (resize per-ch)
    auto ir  = impulseResponse; // copy for worker (mono IR)

    auto safe = juce::Component::SafePointer<MainComponent>(this);

    std::thread([safe, in = std::move(in), out = std::move(out), ir = std::move(ir),
                 numCh, numSmps, dryMix, wetMix, wetGain]() mutable
    {
        juce::ScopedNoDenormals noDenormals;
        juce::FloatVectorOperations::disableDenormalisedNumberSupport();

        const auto start = std::chrono::steady_clock::now();

        // Set block size B and FFT order (K = nextPow2(B+N-1))
        const int N = (int) ir.size();
        const int B = 128; // TODO: Allow for manipulation with the GUI
        int need = B + N - 1;
        int K = juce::nextPowerOfTwo(need);
        int fftOrder = 0; while ((1 << fftOrder) < K) ++fftOrder;

        // Setup final output buffer: Total Length + N - 1
        auto outLen = numSmps + N - 1;
        out->setSize (out->getNumChannels(), outLen);
        out->clear();

        // Progress Bar parameter setup, determine the time it will take so the bar will update correctly
        const double totalWork = (double) numCh * (double) outLen;

        // ===== DEBUG: Measure actual signal levels =====
        float dryPeak = 0.0f;
        float wetPeak = 0.0f;
        
        for (int ch = 0; ch < numCh; ++ch)
        {
            // Create convolver for current channel
            FreqDomainConvolver conv(ir, fftOrder, B);
            conv.reset();

            // Create array cursors
            const float* inPtr  = in->getReadPointer(ch);
            float* outPtr = out->getWritePointer(ch);

            // Move through the input in B-sized chunks
            int processedOut = 0;

            // Main streaming pass (aligned with input timeline)
            for (int i = 0; i < numSmps; i += B)
            {
                const int n = std::min(B, numSmps - i);
                auto yBlock = conv.processBlock(inPtr + i, n); // wet block

                // Wet/Dry mix: y aligns with x at [i -> i+n-1]
                for (int k = 0; k < n; ++k)
                {
                    const float dry = inPtr[i + k];
                    const float wet = yBlock[(size_t) k] * wetGain;
                    
                    dryPeak = std::max(dryPeak, std::abs(dry));
                    wetPeak = std::max(wetPeak, std::abs(wet));
                    
                    outPtr[i + k] = dryMix * dry + wetMix * wet; // Sir-mix-a-lot
                }
                
                // Prep for next block
                processedOut += n;

                if (auto* self = safe.getComponent())
                {
                    const double fraction =
                        ( (double) ch * outLen + (double) processedOut ) / totalWork;
                    self->progressAtomic.store(fraction, std::memory_order_relaxed);
                }
                else { return; } // UI nuked
            }

            // Flush the wet tail and append to end of output buffer
            {
                auto tail = conv.flush(); // exactly N-1 samples
                for (size_t t = 0; t < tail.size(); ++t)
                    outPtr[numSmps + (int) t] = wetMix * tail[t] * wetGain;

                processedOut += (int) tail.size();

                if (auto* self = safe.getComponent())
                {
                    const double fraction =
                        ( (double) ch * outLen + (double) processedOut ) / totalWork;
                    self->progressAtomic.store(fraction, std::memory_order_relaxed);
                }
                else { return; }
            }
        }
        
        DBG("Dry peak: " << dryPeak);
        DBG("Wet peak (with gain): " << wetPeak);
        DBG("Ratio (dry/wet): " << (wetPeak > 0 ? dryPeak / wetPeak : 0));

        const double elapsedSec =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        juce::String durationText = (elapsedSec < 1.0)
            ? juce::String::formatted("%.0f ms", elapsedSec * 1000.0)
            : juce::String(elapsedSec, 2) + " s";

        // Move the finished buffer into the UI thread safely
        auto outOwned = std::move(out);

        juce::MessageManager::callAsync(
            [safe, outOwned = std::move(outOwned),
             durationText = std::move(durationText)]() mutable
            {
                if (auto* self = safe.getComponent())
                {
                    // 1) Swap in the new processed buffer
                    self->audioBuffer = std::move(outOwned);

                    self->waveformProcessed.setBuffer (self->audioBuffer.get(), self->sampleRateHz);

                    if (self->normalizeToggle.getToggleState())
                        self->normalizeToDbFS(-1.0f);

                    self->progressAtomic.store(1.0, std::memory_order_relaxed);
                    self->setStatus("Processing complete (" + durationText + ").");
                    self->updateEnables();
                }
                // else: outOwned auto-destroys safely here
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
                                 24,
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
    exportButton.setEnabled  (hasAudio);
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
