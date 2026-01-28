#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectralConvolverAudioProcessor::SpectralConvolverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
    
    DBG("!!!!!! CONSTRUCTOR CALLED - DEBUG BUILD !!!!!!");
    
    
    juce::Logger::writeToLog("SPECTRAL CONVOLVER LOADED");

    // Create a file as proof the code ran
    juce::File("/tmp/plugin_loaded.txt").replaceWithText("Plugin loaded at " +
        juce::Time::getCurrentTime().toString(true, true));
    
    
//  const int defaultIRLength = 4410; // 100ms at 44.1kHz
//  std::vector<float> defaultIR(defaultIRLength);
//
//
//  for (int i = 0; i < defaultIRLength; ++i) {
//    float t = static_cast<float>(i) / 44100.0f;
//    defaultIR[i] = std::exp(-t * 10.0f) * ((i == 0) ? 1.0f : 0.3f);
//  }
//
//  // Normalize
//  float maxVal = 0.0f;
//  for (auto &s : defaultIR)
//    maxVal = std::max(maxVal, std::abs(s));
//  if (maxVal > 0.0f)
//    for (auto &s : defaultIR)
//      s /= maxVal;
//
    
    
//    const int maxIRLength = 14000;
//    const int defaultIRLength = maxIRLength;
//    std::vector<float> defaultIR(defaultIRLength);
//
//    for (int i = 0; i < defaultIRLength; ++i)
//    {
//        float t = static_cast<float>(i) / 44100.0f;
//        defaultIR[i] = std::exp(-t * 3.0f) * ((i == 0) ? 1.0f : 0.2f);
//    }
//   
//
//    float sumSquares = 0.0f;
//    for (auto& s : defaultIR)
//        sumSquares += s * s;
//    float rms = std::sqrt(sumSquares / maxIRLength);
//
//    // Target RMS of 0.1 (adjust to taste)
//    float gain = 0.1f / rms;
//    for (auto& s : defaultIR)
//        s *= gain;
//    
//    loadImpulseResponse(defaultIR);
    

    
    // Load IR from hardcoded path
    juce::File irFile("/Users/izanagi/repos/SpectralConvolver/Assets/IR.wav");

    if (!loadImpulseResponseFromFile(irFile))
    {
        DBG("Failed to load IR from: " + irFile.getFullPathName());
    }
}g

SpectralConvolverAudioProcessor::~SpectralConvolverAudioProcessor() {}

//==============================================================================
const juce::String SpectralConvolverAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool SpectralConvolverAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool SpectralConvolverAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool SpectralConvolverAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double SpectralConvolverAudioProcessor::getTailLengthSeconds() const {
  // Return the IR length in seconds so hosts know about the reverb tail
  if (irLoaded.load() && currentSampleRate > 0.0)
    return static_cast<double>(irLength) / currentSampleRate;

  return 0.0;
}

int SpectralConvolverAudioProcessor::getNumPrograms() { return 1; }

int SpectralConvolverAudioProcessor::getCurrentProgram() { return 0; }

void SpectralConvolverAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String SpectralConvolverAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void SpectralConvolverAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

//==============================================================================
int SpectralConvolverAudioProcessor::calculateFFTOrder(int irLen,
                                                       int blockSize) {
  // FFT size must be >= blockSize + irLength
  // 1 for overlap-add without aliasing We want the smallest power of 2 that satisfies this
  const int minFFTSize = blockSize + irLen - 1;

  int order = 1;
  while ((1 << order) < minFFTSize)
    ++order;

  // Clamp to reasonable range (64 to 16384)
  order = std::max(6, std::min(14, order));

  return order;
}

void SpectralConvolverAudioProcessor::rebuildConvolvers() {
  const juce::SpinLock::ScopedLockType lock(irLock);

  convolvers.clear();

  if (currentIR.empty())
    return;

  // Calculate optimal FFT order for current settings
  fftOrder = calculateFFTOrder(irLength, currentBlockSize);

  // One convolver per channel
  const int numChannels =
      std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());

  for (int ch = 0; ch < numChannels; ++ch) {
    convolvers.push_back(std::make_unique<FreqDomainConvolver>(
        currentIR, fftOrder, currentBlockSize));
  }

  irLoaded.store(true);
  irPendingRebuild.store(false);

  DBG("Convolvers rebuilt: " << numChannels << " channels, FFT order "
                             << fftOrder << " (size " << (1 << fftOrder)
                             << "), IR length " << irLength);
}

void SpectralConvolverAudioProcessor::prepareToPlay(double sampleRate,
                                                    int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;

  // Rebuild convolvers with new settings
  rebuildConvolvers();
}

void SpectralConvolverAudioProcessor::releaseResources() {
  // Reset convolvers when playback stops
  const juce::SpinLock::ScopedLockType lock(irLock);
  for (auto &conv : convolvers) {
    if (conv)
      conv->reset();
  }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralConvolverAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void SpectralConvolverAudioProcessor::processBlock( juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    
    // FIRST LINE - must see this
    static bool firstCall = true;
    if (firstCall)
    {
      firstCall = false;
      DBG("========== PROCESSBLOCK FIRST CALL ==========");
      DBG("convolvers.size(): " + juce::String(convolvers.size()));
      DBG("currentIR.size(): " + juce::String(currentIR.size()));
      DBG("buffer samples: " + juce::String(buffer.getNumSamples()));
      DBG("=============================================");
    }
    
  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;

  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();
  const auto numSamples = buffer.getNumSamples();
    const auto wetGain = 147.0f;

  // Clear any output channels that don't have corresponding inputs
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, numSamples);

  // Check if IR needs rebuilding
  if (irPendingRebuild.load())
    rebuildConvolvers();

  // If no IR loaded or no convolvers, pass through dry signal
  if (!irLoaded.load() || convolvers.empty())
    return;

  // Try to acquire lock
  // If IR being swapped, pass through
  const juce::SpinLock::ScopedTryLockType lock(irLock);
  if (!lock.isLocked())
    return;

  // Process each channel through respective convolver
  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    if (channel >= static_cast<int>(convolvers.size()) || !convolvers[channel])
      continue;

    auto *channelData = buffer.getWritePointer(channel);

    // Store dry signal if we need it for mixing
    std::vector<float> drySignal;
    if (dryWetMix < 1.0f) {
      drySignal.assign(channelData, channelData + numSamples);
    }
      
      
      
      

    // Process through convolver
    auto wetSignal = convolvers[channel]->processBlock(channelData, numSamples);

      static int debugCounter = 0;
      if (++debugCounter % 200 == 0)  // Log every few seconds
      {
          float wetPeak = 0.0f;
          for (size_t i = 0; i < wetSignal.size(); ++i)
              wetPeak = std::max(wetPeak, std::abs(wetSignal[i]));
          
          DBG("wetSignal size: " + juce::String(wetSignal.size()) +
              ", peak: " + juce::String(wetPeak) +
              ", numSamples: " + juce::String(numSamples));
      }
      
      if (++debugCounter % 200 == 0)
      {
          bool hasNaN = false;
          bool hasInf = false;
          float maxVal = 0.0f;
          
          for (size_t i = 0; i < wetSignal.size(); ++i)
          {
              if (std::isnan(wetSignal[i])) hasNaN = true;
              if (std::isinf(wetSignal[i])) hasInf = true;
              maxVal = std::max(maxVal, std::abs(wetSignal[i]));
          }
          
          DBG("wetSignal - size: " + juce::String(wetSignal.size()) +
              ", max: " + juce::String(maxVal) +
              ", NaN: " + juce::String(hasNaN ? "YES" : "no") +
              ", Inf: " + juce::String(hasInf ? "no" : "no"));
      }
      
      
      
    const int outputSize =
        std::min(static_cast<int>(wetSignal.size()), numSamples);

    // Copy result back to buffer with dry/wet mix
      if (dryWetMix >= 1.0f)
      {
        // 100% wet - just copy
        for (int i = 0; i < outputSize; ++i)
          channelData[i] = wetSignal[i] * wetGain;
        
        // DEBUG: Verify it actually got copied
        static int copyCheck = 0;
        if (++copyCheck % 200 == 0)
        {
          float bufferPeak = 0.0f;
          for (int i = 0; i < outputSize; ++i)
            bufferPeak = std::max(bufferPeak, std::abs(channelData[i]));
          DBG("AFTER COPY - buffer peak: " + juce::String(bufferPeak));
        }
      }
    else if (dryWetMix <= 0.0f) {
      // 100% dry - leave buffer unchanged (already has dry signal)
    }
    else
    {
      // Mix dry and wet
      const float wet = dryWetMix;
      const float dry = 1.0f - dryWetMix;

      for (int i = 0; i < outputSize; ++i)
        channelData[i] = dry * drySignal[i] + (wet * wetSignal[i] * wetGain);
    }
      
    // Zero any remaining samples
    for (int i = outputSize; i < numSamples; ++i)
      channelData[i] = 0.0f;
  }
}

void SpectralConvolverAudioProcessor::loadImpulseResponse(
    const std::vector<float> &ir) {
  if (ir.empty())
    return;

  {
    const juce::SpinLock::ScopedLockType lock(irLock);
    currentIR = ir;
    irLength = static_cast<int>(ir.size());
  }

  // Flag for rebuild on audio thread (safer than rebuilding here)
  irPendingRebuild.store(true);

  DBG("IR loaded: " << irLength << " samples");
}

bool SpectralConvolverAudioProcessor::loadImpulseResponseFromFile(
    const juce::File &file) {
  if (!file.existsAsFile())
    return false;

  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(file));

  if (!reader)
    return false;

  // Read the IR (use first channel if stereo, or load both for true stereo
  // later)
  const int numSamples = static_cast<int>(reader->lengthInSamples);

  if (numSamples <= 0 || numSamples > 10 * 48000) // Max 10 seconds
    return false;

  juce::AudioBuffer<float> irBuffer(1, numSamples);
  reader->read(&irBuffer, 0, numSamples, 0, true, false);

  // Convert to vector
  std::vector<float> ir(irBuffer.getReadPointer(0),
                        irBuffer.getReadPointer(0) + numSamples);

  loadImpulseResponse(ir);

  return true;
}

bool SpectralConvolverAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *SpectralConvolverAudioProcessor::createEditor() {
  return new SpectralConvolverAudioProcessorEditor(*this);
}

void SpectralConvolverAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  // Save IR path or data if needed
  // For now, just save dry/wet mix
  juce::MemoryOutputStream stream(destData, true);
  stream.writeFloat(dryWetMix);
}

void SpectralConvolverAudioProcessor::setStateInformation(const void *data,
                                                          int sizeInBytes) {
  juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
  if (sizeInBytes >= sizeof(float))
    dryWetMix = stream.readFloat();
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new SpectralConvolverAudioProcessor();
}
