/*
  ==============================================================================

    ImpulseResponseSelector.cpp
    Created: 22 Sep 2025 10:13:43am
    Author:  izanagi

  ==============================================================================
*/

#include "ImpulseResponseSelector.h"

ImpulseResponseSelector::ImpulseResponseSelector()
{
    formatManager.registerBasicFormats();

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Impulse Response", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(loadFolderButton);
    loadFolderButton.setButtonText("Load IR Folder...");
    loadFolderButton.onClick = [this]
    {
        folderChooser = std::make_unique<juce::FileChooser>(
            "Select a folder containing .wav impulse responses",
            juce::File{},
            juce::String{}
    );

            auto flags = juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectDirectories;

            juce::Component::SafePointer<ImpulseResponseSelector> safeThis(this);
            folderChooser->launchAsync(flags, [safeThis](const juce::FileChooser& fc)
            {
                if (safeThis == nullptr) return;
                auto dir = fc.getResult();
                if (dir.isDirectory())
                    safeThis->loadIRsFromDirectory(dir);

                safeThis->folderChooser.reset();
            });
        };

    addAndMakeVisible(irCombo);
    irCombo.onChange = [this]
        {
            selectedIndex = irCombo.getSelectedItemIndex();
            if (onSelectionChanged) onSelectionChanged(selectedIndex);
        };
    irCombo.setTextWhenNothingSelected("No IR loaded");
}

void ImpulseResponseSelector::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto top = area.removeFromTop(24);
    titleLabel.setBounds(top.removeFromLeft(180));
    loadFolderButton.setBounds(top.removeFromRight(160));
    area.removeFromTop(6);
    irCombo.setBounds(area.removeFromTop(28));
}

void ImpulseResponseSelector::loadIRsFromDirectory(const juce::File& directory)
{
    // Get *.wav files
    juce::Array<juce::File> files;
    directory.findChildFiles(files, juce::File::findFiles, true, "*.wav");

    // Clear existing
    irs.clear();
    irCombo.clear(juce::dontSendNotification);
    selectedIndex = -1;

    int nextId = 1;

    for (auto& f : files)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(f));
        if (reader == nullptr || reader->lengthInSamples <= 0)
            continue;

        auto item = std::make_unique<IRItem>();
        item->name = f.getFileNameWithoutExtension();
        item->sampleRate = reader->sampleRate;
        item->numChannels = (int)reader->numChannels;
        item->sourceFile = f;

        const int numSamples = (int)juce::jlimit<int>(0, std::numeric_limits<int>::max(), reader->lengthInSamples);
        item->buffer.setSize(item->numChannels, numSamples);
        item->buffer.clear();

        if (reader->read(&item->buffer, 0, numSamples, 0, true, true))
        {
            irCombo.addItem(item->name, nextId++);
            irs.push_back(std::move(item));
        }
    }

    if (!irs.empty())
    {
        irCombo.setSelectedItemIndex(0, juce::sendNotificationSync);
        selectedIndex = 0;
    }
    else
    {
        irCombo.setText("No .wav files found", juce::dontSendNotification);
        selectedIndex = -1;
    }

    if (onListRefreshed) onListRefreshed();
}