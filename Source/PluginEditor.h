#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VUMisterNAMAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit VUMisterNAMAudioProcessorEditor (VUMisterNAMAudioProcessor&);
    ~VUMisterNAMAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateModelStatus();
    void styleSlider (juce::Slider&, const juce::String&);

    VUMisterNAMAudioProcessor& audioProcessor;
    juce::Image backgroundImage;
    juce::Slider inputSlider, outputSlider, mixSlider;

    // Invisible hit target: unlike a JUCE Button it cannot paint a focus/checkbox
    // frame over the artwork in hosts such as Nuendo 15.
    class BypassHitTarget final : public juce::Component
    {
    public:
        std::function<void()> onClick;
        void mouseUp (const juce::MouseEvent& event) override
        {
            if (event.mouseWasClicked() && onClick)
                onClick();
        }
        void paint (juce::Graphics&) override {}
    } bypassButton;
    juce::TextButton loadButton { "LOAD NAM" };
    juce::ComboBox modelSelector;
    juce::Label modelLabel, statusLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMisterNAMAudioProcessorEditor)
};
