/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginAlgorithm.h"
#include "ImpulseResponseSelector.h"


//==============================================================================
TDConvolveAudioProcessor::TDConvolveAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    formatManager.registerBasicFormats();
}

TDConvolveAudioProcessor::~TDConvolveAudioProcessor()
{
}

//==============================================================================
const juce::String TDConvolveAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TDConvolveAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TDConvolveAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TDConvolveAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TDConvolveAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TDConvolveAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TDConvolveAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TDConvolveAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TDConvolveAudioProcessor::getProgramName (int index)
{
    return {};
}

void TDConvolveAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TDConvolveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void TDConvolveAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TDConvolveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void TDConvolveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

	// Convert the IR data to a format the convolver can use
	std::vector <float> ir;
    int irSize = currentIR->buffer.getNumSamples();

    for (int i = 0; i < irSize; ++i)
        ir.push_back(currentIR->buffer.getSample(0, i)); // mono IR

	//Create the convolver
    TimeDomainConvolver convolver(ir);

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {

        juce::AudioBuffer<float> tempBuffer = buffer;
        auto* inputData = tempBuffer.getWritePointer (channel);

        auto* outputData = buffer.getWritePointer(channel);

		convolver.processBlock(inputData, outputData,  buffer.getNumSamples());
    }

    return;
}

//==============================================================================
bool TDConvolveAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TDConvolveAudioProcessor::createEditor()
{
    return new TDConvolveAudioProcessorEditor (*this);
}

//==============================================================================
void TDConvolveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TDConvolveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

bool TDConvolveAudioProcessor::loadWavFile(const juce::File& file)
{
    fileLoaded = false;
    fileBuffer.setSize(0, 0);
    fileSampleRate = 0.0;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;

    const int length = (int)reader->lengthInSamples;
    if (length <= 0) return false;

    fileSampleRate = reader->sampleRate;

    // allocate buffer with the file's channel count and length
    fileBuffer.setSize((int)reader->numChannels, (int)length);
    fileBuffer.clear();

    // read entire file into buffer
    const bool ok = reader->read(&fileBuffer,              // dest buffer
        0,                                                 // dest start sample
        (int)length,                                       // num samples
        0,                                                 // reader start sample
        true,                                              // use left channel?
        true);                                             // use right channel?
    fileLoaded = ok;
    return ok;
}

bool TDConvolveAudioProcessor::exportProcessedWav(const juce::File& outFile, int blockSize)
{
    if (!fileLoaded || fileBuffer.getNumSamples() == 0)
        return false;

    // Make sure bus layout matches how processBlock will be called
    const int inCh = getTotalNumInputChannels();
    const int outCh = getTotalNumOutputChannels();

    if (inCh == 0 || outCh == 0)
        return false;

    // Prepare processor at the file's sample rate
    prepareToPlay(fileSampleRate, blockSize);

    const int numSamples = fileBuffer.getNumSamples();

    // Output buffer sized to outCh and total samples
    juce::AudioBuffer<float> outBuffer(outCh, numSamples);
    outBuffer.clear();

    juce::MidiBuffer dummyMidi;

    // Block buffer that matches IO channel counts
    juce::AudioBuffer<float> workBuffer(std::max(inCh, outCh), blockSize);

    for (int pos = 0; pos < numSamples; pos += blockSize)
    {
        const int thisBlock = std::min(blockSize, numSamples - pos);

        // Resize work buffer for last partial block
        workBuffer.setSize(std::max(inCh, outCh), thisBlock, false, false, true);
        workBuffer.clear();

        // Copy file input into the processor's input channels
        for (int ch = 0; ch < inCh; ++ch)
        {
            const int srcCh = juce::jmin(ch, fileBuffer.getNumChannels() - 1);
            workBuffer.copyFrom(ch, 0, fileBuffer, srcCh, pos, thisBlock);
        }

        // DSP
        processBlock(workBuffer, dummyMidi);

        // Copy processed result to outBuffer
        for (int ch = 0; ch < outCh; ++ch)
        {
            const int srcCh = juce::jmin(ch, workBuffer.getNumChannels() - 1);
            outBuffer.copyFrom(ch, pos, workBuffer, srcCh, 0, thisBlock);
        }
    }

    releaseResources();

    // Write outBuffer to WAV
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream(outFile.createOutputStream());

    if (!stream || !stream->openedOk())
        return false;

    const int bitsPerSample = 24;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wav.createWriterFor(stream.get(),
            fileSampleRate,
            (unsigned int)outCh,
            bitsPerSample,
            {}, 0));

    if (!writer) return false;

    stream.release();

    const bool wrote = writer->writeFromAudioSampleBuffer(outBuffer, 0, outBuffer.getNumSamples());

    return wrote;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TDConvolveAudioProcessor();
}
