/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TDConvolveAudioProcessorEditor::TDConvolveAudioProcessorEditor (TDConvolveAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    addAndMakeVisible(loadBtn);
    addAndMakeVisible(exportBtn);
	addAndMakeVisible(irSelector);

    loadBtn.onClick = [this]
    {
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Select a WAV to load", juce::File{}, "*.wav");

            auto flags = juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectFiles;

            // Guard against the editor being closed while the dialog is up
            juce::Component::SafePointer<TDConvolveAudioProcessorEditor> safeThis(this);

            fileChooser->launchAsync(flags, [safeThis](const juce::FileChooser& fc)
            {
                if (safeThis == nullptr) return;

                juce::File file = fc.getResult();
                if (file.existsAsFile())
                {
                    if (!safeThis->audioProcessor.loadWavFile(file))
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                            "Load Failed", "Could not read that WAV file.");
                }

                safeThis->fileChooser.reset(); // release after callback
            });
        };
    };

    exportBtn.onClick = [this]
    {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Choose where to save the processed WAV",
                juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                "*.wav");

            auto flags = juce::FileBrowserComponent::saveMode
                | juce::FileBrowserComponent::canSelectFiles;

            juce::Component::SafePointer<TDConvolveAudioProcessorEditor> safeThis(this);

            fileChooser->launchAsync(flags, [safeThis](const juce::FileChooser& fc)
            {
                if (safeThis == nullptr) return;

                juce::File outFile = fc.getResult().withFileExtension(".wav");
                if (outFile == juce::File{}) return;

                if (!safeThis->audioProcessor.exportProcessedWav(outFile))
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "Export Failed", "Could not write the WAV file.");

                safeThis->fileChooser.reset();
            });
    };

    irSelector.onSelectionChanged = [this](int)
    {
        if (auto* item = irSelector.getSelectedIR())
            audioProcessor.setCurrentIR(item);
    };


}

TDConvolveAudioProcessorEditor::~TDConvolveAudioProcessorEditor()
{
}

//==============================================================================
void TDConvolveAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
}

void TDConvolveAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto area = getLocalBounds().reduced(10);
    loadBtn.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);
    exportBtn.setBounds(area.removeFromTop(30));

    irSelector.setBounds(10, 80, getWidth() - 20, 80);
}
