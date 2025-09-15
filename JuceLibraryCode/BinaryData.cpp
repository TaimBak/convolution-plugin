/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== PluginProcessor.cpp ==================
static const unsigned char temp_binary_data_0[] =
"/*\r\n"
"  ==============================================================================\r\n"
"\r\n"
"    This file contains the basic framework code for a JUCE plugin processor.\r\n"
"\r\n"
"  ==============================================================================\r\n"
"*/\r\n"
"\r\n"
"#include \"PluginProcessor.h\"\r\n"
"#include \"PluginEditor.h\"\r\n"
"\r\n"
"//==============================================================================\r\n"
"TDConvolveAudioProcessor::TDConvolveAudioProcessor()\r\n"
"#ifndef JucePlugin_PreferredChannelConfigurations\r\n"
"     : AudioProcessor (BusesProperties()\r\n"
"                     #if ! JucePlugin_IsMidiEffect\r\n"
"                      #if ! JucePlugin_IsSynth\r\n"
"                       .withInput  (\"Input\",  juce::AudioChannelSet::stereo(), true)\r\n"
"                      #endif\r\n"
"                       .withOutput (\"Output\", juce::AudioChannelSet::stereo(), true)\r\n"
"                     #endif\r\n"
"                       )\r\n"
"#endif\r\n"
"{\r\n"
"}\r\n"
"\r\n"
"TDConvolveAudioProcessor::~TDConvolveAudioProcessor()\r\n"
"{\r\n"
"}\r\n"
"\r\n"
"//==============================================================================\r\n"
"const juce::String TDConvolveAudioProcessor::getName() const\r\n"
"{\r\n"
"    return JucePlugin_Name;\r\n"
"}\r\n"
"\r\n"
"bool TDConvolveAudioProcessor::acceptsMidi() const\r\n"
"{\r\n"
"   #if JucePlugin_WantsMidiInput\r\n"
"    return true;\r\n"
"   #else\r\n"
"    return false;\r\n"
"   #endif\r\n"
"}\r\n"
"\r\n"
"bool TDConvolveAudioProcessor::producesMidi() const\r\n"
"{\r\n"
"   #if JucePlugin_ProducesMidiOutput\r\n"
"    return true;\r\n"
"   #else\r\n"
"    return false;\r\n"
"   #endif\r\n"
"}\r\n"
"\r\n"
"bool TDConvolveAudioProcessor::isMidiEffect() const\r\n"
"{\r\n"
"   #if JucePlugin_IsMidiEffect\r\n"
"    return true;\r\n"
"   #else\r\n"
"    return false;\r\n"
"   #endif\r\n"
"}\r\n"
"\r\n"
"double TDConvolveAudioProcessor::getTailLengthSeconds() const\r\n"
"{\r\n"
"    return 0.0;\r\n"
"}\r\n"
"\r\n"
"int TDConvolveAudioProcessor::getNumPrograms()\r\n"
"{\r\n"
"    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,\r\n"
"                // so this should be at least 1, even if you're not really implementing programs.\r\n"
"}\r\n"
"\r\n"
"int TDConvolveAudioProcessor::getCurrentProgram()\r\n"
"{\r\n"
"    return 0;\r\n"
"}\r\n"
"\r\n"
"void TDConvolveAudioProcessor::setCurrentProgram (int index)\r\n"
"{\r\n"
"}\r\n"
"\r\n"
"const juce::String TDConvolveAudioProcessor::getProgramName (int index)\r\n"
"{\r\n"
"    return {};\r\n"
"}\r\n"
"\r\n"
"void TDConvolveAudioProcessor::changeProgramName (int index, const juce::String& newName)\r\n"
"{\r\n"
"}\r\n"
"\r\n"
"//==============================================================================\r\n"
"void TDConvolveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)\r\n"
"{\r\n"
"    // Use this method as the place to do any pre-playback\r\n"
"    // initialisation that you need..\r\n"
"}\r\n"
"\r\n"
"void TDConvolveAudioProcessor::releaseResources()\r\n"
"{\r\n"
"    // When playback stops, you can use this as an opportunity to free up any\r\n"
"    // spare memory, etc.\r\n"
"}\r\n"
"\r\n"
"#ifndef JucePlugin_PreferredChannelConfigurations\r\n"
"bool TDConvolveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const\r\n"
"{\r\n"
"  #if JucePlugin_IsMidiEffect\r\n"
"    juce::ignoreUnused (layouts);\r\n"
"    return true;\r\n"
"  #else\r\n"
"    // This is the place where you check if the layout is supported.\r\n"
"    // In this template code we only support mono or stereo.\r\n"
"    // Some plugin hosts, such as certain GarageBand versions, will only\r\n"
"    // load plugins that support stereo bus layouts.\r\n"
"    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()\r\n"
"     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())\r\n"
"        return false;\r\n"
"\r\n"
"    // This checks if the input layout matches the output layout\r\n"
"   #if ! JucePlugin_IsSynth\r\n"
"    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())\r\n"
"        return false;\r\n"
"   #endif\r\n"
"\r\n"
"    return true;\r\n"
"  #endif\r\n"
"}\r\n"
"#endif\r\n"
"\r\n"
"void TDConvolveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)\r\n"
"{\r\n"
"    juce::ScopedNoDenormals noDenormals;\r\n"
"    auto totalNumInputChannels  = getTotalNumInputChannels();\r\n"
"    auto totalNumOutputChannels = getTotalNumOutputChannels();\r\n"
"\r\n"
"    // In case we have more outputs than inputs, this code clears any output\r\n"
"    // channels that didn't contain input data, (because these aren't\r\n"
"    // guaranteed to be empty - they may contain garbage).\r\n"
"    // This is here to avoid people getting screaming feedback\r\n"
"    // when they first compile a plugin, but obviously you don't need to keep\r\n"
"    // this code if your algorithm always overwrites all the output channels.\r\n"
"    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)\r\n"
"        buffer.clear (i, 0, buffer.getNumSamples());\r\n"
"\r\n"
"    // This is the place where you'd normally do the guts of your plugin's\r\n"
"    // audio processing...\r\n"
"    // Make sure to reset the state if your inner loop is processing\r\n"
"    // the samples and the outer loop is handling the channels.\r\n"
"    // Alternatively, you can process the samples with the channels\r\n"
"    // interleaved by keeping the same state.\r\n"
"    for (int channel = 0; channel < totalNumInputChannels; ++channel)\r\n"
"    {\r\n"
"        auto* channelData = buffer.getWritePointer (channel);\r\n"
"\r\n"
"        // ..do something to the data...\r\n"
"    }\r\n"
"}\r\n"
"\r\n"
"//==============================================================================\r\n"
"bool TDConvolveAudioProcessor::hasEditor() const\r\n"
"{\r\n"
"    return true; // (change this to false if you choose to not supply an editor)\r\n"
"}\r\n"
"\r\n"
"juce::AudioProcessorEditor* TDConvolveAudioProcessor::createEditor()\r\n"
"{\r\n"
"    return new TDConvolveAudioProcessorEditor (*this);\r\n"
"}\r\n"
"\r\n"
"//==============================================================================\r\n"
"void TDConvolveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)\r\n"
"{\r\n"
"    // You should use this method to store your parameters in the memory block.\r\n"
"    // You could do that either as raw data, or use the XML or ValueTree classes\r\n"
"    // as intermediaries to make it easy to save and load complex data.\r\n"
"}\r\n"
"\r\n"
"void TDConvolveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)\r\n"
"{\r\n"
"    // You should use this method to restore your parameters from this memory block,\r\n"
"    // whose contents will have been created by the getStateInformation() call.\r\n"
"}\r\n"
"\r\n"
"//==============================================================================\r\n"
"// This creates new instances of the plugin..\r\n"
"juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()\r\n"
"{\r\n"
"    return new TDConvolveAudioProcessor();\r\n"
"}\r\n";

const char* PluginProcessor_cpp = (const char*) temp_binary_data_0;

//================== PluginProcessor.h ==================
static const unsigned char temp_binary_data_1[] =
"/*\r\n"
"  ==============================================================================\r\n"
"\r\n"
"    This file contains the basic framework code for a JUCE plugin processor.\r\n"
"\r\n"
"  ==============================================================================\r\n"
"*/\r\n"
"\r\n"
"#pragma once\r\n"
"\r\n"
"#include <JuceHeader.h>\r\n"
"\r\n"
"//==============================================================================\r\n"
"/**\r\n"
"*/\r\n"
"class TDConvolveAudioProcessor  : public juce::AudioProcessor\r\n"
"{\r\n"
"public:\r\n"
"    //==============================================================================\r\n"
"    TDConvolveAudioProcessor();\r\n"
"    ~TDConvolveAudioProcessor() override;\r\n"
"\r\n"
"    //==============================================================================\r\n"
"    void prepareToPlay (double sampleRate, int samplesPerBlock) override;\r\n"
"    void releaseResources() override;\r\n"
"\r\n"
"   #ifndef JucePlugin_PreferredChannelConfigurations\r\n"
"    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;\r\n"
"   #endif\r\n"
"\r\n"
"    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;\r\n"
"\r\n"
"    //==============================================================================\r\n"
"    juce::AudioProcessorEditor* createEditor() override;\r\n"
"    bool hasEditor() const override;\r\n"
"\r\n"
"    //==============================================================================\r\n"
"    const juce::String getName() const override;\r\n"
"\r\n"
"    bool acceptsMidi() const override;\r\n"
"    bool producesMidi() const override;\r\n"
"    bool isMidiEffect() const override;\r\n"
"    double getTailLengthSeconds() const override;\r\n"
"\r\n"
"    //==============================================================================\r\n"
"    int getNumPrograms() override;\r\n"
"    int getCurrentProgram() override;\r\n"
"    void setCurrentProgram (int index) override;\r\n"
"    const juce::String getProgramName (int index) override;\r\n"
"    void changeProgramName (int index, const juce::String& newName) override;\r\n"
"\r\n"
"    //==============================================================================\r\n"
"    void getStateInformation (juce::MemoryBlock& destData) override;\r\n"
"    void setStateInformation (const void* data, int sizeInBytes) override;\r\n"
"\r\n"
"private:\r\n"
"    //==============================================================================\r\n"
"    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDConvolveAudioProcessor)\r\n"
"};\r\n";

const char* PluginProcessor_h = (const char*) temp_binary_data_1;

//================== PluginEditor.cpp ==================
static const unsigned char temp_binary_data_2[] =
"/*\r\n"
"  ==============================================================================\r\n"
"\r\n"
"    This file contains the basic framework code for a JUCE plugin editor.\r\n"
"\r\n"
"  ==============================================================================\r\n"
"*/\r\n"
"\r\n"
"#include \"PluginProcessor.h\"\r\n"
"#include \"PluginEditor.h\"\r\n"
"\r\n"
"//==============================================================================\r\n"
"TDConvolveAudioProcessorEditor::TDConvolveAudioProcessorEditor (TDConvolveAudioProcessor& p)\r\n"
"    : AudioProcessorEditor (&p), audioProcessor (p)\r\n"
"{\r\n"
"    // Make sure that before the constructor has finished, you've set the\r\n"
"    // editor's size to whatever you need it to be.\r\n"
"    setSize (400, 300);\r\n"
"}\r\n"
"\r\n"
"TDConvolveAudioProcessorEditor::~TDConvolveAudioProcessorEditor()\r\n"
"{\r\n"
"}\r\n"
"\r\n"
"//==============================================================================\r\n"
"void TDConvolveAudioProcessorEditor::paint (juce::Graphics& g)\r\n"
"{\r\n"
"    // (Our component is opaque, so we must completely fill the background with a solid colour)\r\n"
"    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));\r\n"
"\r\n"
"    g.setColour (juce::Colours::white);\r\n"
"    g.setFont (juce::FontOptions (15.0f));\r\n"
"    g.drawFittedText (\"Hello World!\", getLocalBounds(), juce::Justification::centred, 1);\r\n"
"}\r\n"
"\r\n"
"void TDConvolveAudioProcessorEditor::resized()\r\n"
"{\r\n"
"    // This is generally where you'll want to lay out the positions of any\r\n"
"    // subcomponents in your editor..\r\n"
"}\r\n";

const char* PluginEditor_cpp = (const char*) temp_binary_data_2;

//================== PluginEditor.h ==================
static const unsigned char temp_binary_data_3[] =
"/*\r\n"
"  ==============================================================================\r\n"
"\r\n"
"    This file contains the basic framework code for a JUCE plugin editor.\r\n"
"\r\n"
"  ==============================================================================\r\n"
"*/\r\n"
"\r\n"
"#pragma once\r\n"
"\r\n"
"#include <JuceHeader.h>\r\n"
"#include \"PluginProcessor.h\"\r\n"
"\r\n"
"//==============================================================================\r\n"
"/**\r\n"
"*/\r\n"
"class TDConvolveAudioProcessorEditor  : public juce::AudioProcessorEditor\r\n"
"{\r\n"
"public:\r\n"
"    TDConvolveAudioProcessorEditor (TDConvolveAudioProcessor&);\r\n"
"    ~TDConvolveAudioProcessorEditor() override;\r\n"
"\r\n"
"    //==============================================================================\r\n"
"    void paint (juce::Graphics&) override;\r\n"
"    void resized() override;\r\n"
"\r\n"
"private:\r\n"
"    // This reference is provided as a quick way for your editor to\r\n"
"    // access the processor object that created it.\r\n"
"    TDConvolveAudioProcessor& audioProcessor;\r\n"
"\r\n"
"    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDConvolveAudioProcessorEditor)\r\n"
"};\r\n";

const char* PluginEditor_h = (const char*) temp_binary_data_3;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x687677e3:  numBytes = 6333; return PluginProcessor_cpp;
        case 0xe1bd86a8:  numBytes = 2319; return PluginProcessor_h;
        case 0xd36ebf84:  numBytes = 1465; return PluginEditor_cpp;
        case 0xebb54289:  numBytes = 1050; return PluginEditor_h;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "PluginProcessor_cpp",
    "PluginProcessor_h",
    "PluginEditor_cpp",
    "PluginEditor_h"
};

const char* originalFilenames[] =
{
    "PluginProcessor.cpp",
    "PluginProcessor.h",
    "PluginEditor.cpp",
    "PluginEditor.h"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
