/*
  ==============================================================================

    ImpulseResponseSelector.h
    Created: 22 Sep 2025 10:13:43am
    Author:  izanagi

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

class ImpulseResponseSelector : public juce::Component
{
public:
    struct IRItem
    {
        juce::String name;
        juce::AudioBuffer<float> buffer;
        double sampleRate = 0.0;
        int numChannels = 0;
        juce::File sourceFile;
    };

    ImpulseResponseSelector();

    // === Public API ===
    int getNumIRs() const noexcept { return (int)irs.size(); }
    int getSelectedIndex() const noexcept { return selectedIndex; }
    const IRItem* getIR(int index) const noexcept
    {
        if (index < 0 || index >= (int)irs.size()) return nullptr;
        return irs[(size_t)index].get();
    }
    const IRItem* getSelectedIR() const noexcept { return getIR(getSelectedIndex()); }

    // Optional callbacks
    std::function<void(int newIndex)> onSelectionChanged;
    std::function<void()> onListRefreshed;

    void resized() override;

private:
    void loadIRsFromDirectory(const juce::File& directory);

    // UI
    juce::Label titleLabel;
    juce::TextButton loadFolderButton;
    juce::ComboBox irCombo;

    // Data
    juce::AudioFormatManager formatManager;
    std::vector<std::unique_ptr<IRItem>> irs;
    int selectedIndex = -1;

    // Async chooser must stay alive while open
    std::unique_ptr<juce::FileChooser> folderChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImpulseResponseSelector)
};
