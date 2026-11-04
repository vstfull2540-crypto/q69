#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    const juce::Colour amber (0xffffa51f);
    const juce::Colour cream (0xffffe9b0);
    const juce::Colour panel (0xff151718);

    void drawMetalKnobWell (juce::Graphics& g, juce::Rectangle<float> r)
    {
        // Cover the baked-in knob completely before the live JUCE rotary is painted.
        // The outer rim is slightly larger than the static knob in the reference image.
        g.setColour (juce::Colour (0xff030405));
        g.fillEllipse (r.expanded (3.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35393a), r.getX(), r.getY(),
                                                 juce::Colour (0xff080a0b), r.getRight(), r.getBottom(), false));
        g.fillEllipse (r);
        g.setColour (juce::Colour (0xff65696a).withAlpha (0.65f));
        g.drawEllipse (r.reduced (1.5f), 1.1f);
    }
}

VUMisterNAMAudioProcessorEditor::VUMisterNAMAudioProcessorEditor (VUMisterNAMAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::reference_full_interface_png,
                                                        BinaryData::reference_full_interface_pngSize);
    setSize (1100, 602);
    setResizable (true, true);
    setResizeLimits (950, 520, 1586, 868);

    auto setupSlider = [this] (juce::Slider& s, const juce::String& name)
    {
        s.setName (name);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        // The faceplate already contains the INPUT / MIX / OUTPUT legends.
        // A JUCE text box here overlaps those legends at common plugin sizes.
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::rotarySliderFillColourId, amber);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff646767));
        s.setColour (juce::Slider::thumbColourId, cream);
        s.setColour (juce::Slider::textBoxTextColourId, cream);
        s.setColour (juce::Slider::textBoxBackgroundColourId, panel);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff56595a));
        addAndMakeVisible (s);
    };
    setupSlider (inputSlider, "INPUT");
    setupSlider (mixSlider, "MIX");
    setupSlider (outputSlider, "OUTPUT");

    inputSlider.setRange (-24.0, 24.0, 0.1);
    outputSlider.setRange (-24.0, 24.0, 0.1);
    mixSlider.setRange (0.0, 100.0, 0.1);
    inputSlider.setTextValueSuffix (" dB");
    outputSlider.setTextValueSuffix (" dB");
    mixSlider.setTextValueSuffix (" %");
    inputSlider.setValue (p.parameters.getRawParameterValue ("input")->load());
    outputSlider.setValue (p.parameters.getRawParameterValue ("output")->load());
    mixSlider.setValue (p.parameters.getRawParameterValue ("mix")->load());

    inputSlider.onValueChange = [this] { audioProcessor.parameters.getParameterAsValue ("input").setValue (inputSlider.getValue()); };
    outputSlider.onValueChange = [this] { audioProcessor.parameters.getParameterAsValue ("output").setValue (outputSlider.getValue()); };
    mixSlider.onValueChange = [this] { audioProcessor.parameters.getParameterAsValue ("mix").setValue (mixSlider.getValue()); };

    // Invisible hit targets keep host-specific button chrome away from the artwork.
    // Disable keyboard focus as well, so Nuendo cannot draw a focus rectangle.
    bypassButton.setWantsKeyboardFocus (false);
    bypassButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    bypassButton.onClick = [this]
    {
        auto* bypassParameter = audioProcessor.parameters.getRawParameterValue ("bypass");
        const bool nextState = bypassParameter->load() <= 0.5f;
        audioProcessor.parameters.getParameterAsValue ("bypass").setValue (nextState);
        repaint();
    };
    addAndMakeVisible (bypassButton);

    // The LOAD NAM label and border are already painted into the reference artwork.
    // Keep a transparent clickable TextButton so its JUCE border/text never doubles them.
    loadButton.setButtonText (juce::String{});
    loadButton.setWantsKeyboardFocus (false);
    loadButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    loadButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    loadButton.setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    loadButton.setColour (juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Choose a NAM model", juce::File{}, "*.nam");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();
                if (file != juce::File{})
                {
                    audioProcessor.loadNamFile (file);
                    updateModelStatus();
                }
            });
    };
    addAndMakeVisible (loadButton);

    // The faceplate already contains the selector well. Make the ComboBox fill and
    // outline transparent to avoid a second rectangle, while retaining live text/arrow.
    modelSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101213));
    modelSelector.setColour (juce::ComboBox::textColourId, cream);
    modelSelector.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    modelSelector.setColour (juce::ComboBox::arrowColourId, cream);
    modelSelector.setWantsKeyboardFocus (false);
    modelSelector.addItem ("UREI 1176-SF - Fast Control.nam", 1);
    modelSelector.setSelectedId (1, juce::dontSendNotification);
    modelSelector.onChange = [this]
    {
        const auto model = juce::File (juce::File::getSpecialLocation (juce::File::currentApplicationFile)
                                         .getParentDirectory().getChildFile ("Models/UREI 1176-SF - Fast Control.nam"));
        if (model.existsAsFile()) audioProcessor.loadNamFile (model);
        updateModelStatus();
    };
    addAndMakeVisible (modelSelector);

    modelLabel.setColour (juce::Label::textColourId, cream);
    modelLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    modelLabel.setJustificationType (juce::Justification::centredLeft);
    // Do not display these status labels below the controls: that area is part of
    // the fixed artwork and their text overlapped the model selector/load button.
    // The same information is exposed as tooltips instead.
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd6d0c3));
    statusLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    updateModelStatus();
    startTimerHz (30);
}

void VUMisterNAMAudioProcessorEditor::styleSlider (juce::Slider&, const juce::String&) {}

void VUMisterNAMAudioProcessorEditor::updateModelStatus()
{
    const auto path = audioProcessor.getNamPath();
    modelLabel.setText (path.isNotEmpty() ? juce::File (path).getFileName() : "No NAM model loaded",
                        juce::dontSendNotification);
    statusLabel.setText (audioProcessor.getNamStatus(), juce::dontSendNotification);
    modelSelector.setTooltip (path.isNotEmpty() ? juce::File (path).getFileName()
                                                : "No NAM model loaded");
    loadButton.setTooltip (audioProcessor.getNamStatus());
}

void VUMisterNAMAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (panel);
    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, getLocalBounds().toFloat(),
                     juce::RectanglePlacement::stretchToFit);
    else
    {
        g.setColour (cream);
        g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
        g.drawText ("VUMISTER NAM", getLocalBounds(), juce::Justification::centred);
    }

    // Mask the static artwork knobs so the live rotary controls are visible and interactive.
    const float sx = (float) getWidth() / 1522.0f;
    const float sy = (float) getHeight() / 833.0f;
    for (const auto centre : { juce::Point<float> (212.0f * sx, 565.0f * sy),
                               juce::Point<float> (574.0f * sx, 565.0f * sy),
                               juce::Point<float> (942.0f * sx, 565.0f * sy) })
        drawMetalKnobWell (g, { centre.x - 76.0f * sx, centre.y - 76.0f * sy,
                                152.0f * sx, 152.0f * sy });

    // Draw real-time VU needles over the static faceplate artwork. Meter values
    // are published atomically by processBlock(), so the editor never touches audio buffers.
    const auto drawNeedle = [&g] (float db, juce::Point<float> pivot, float scale)
    {
        // Match the printed arc from -20 to +3 VU. The larger sweep and longer
        // pointer follow the artwork's real tick arc instead of stopping short.
        const float level = juce::jlimit (-20.0f, 3.0f, db);
        const float proportion = (level + 20.0f) / 23.0f;
        const float angle = (-160.0f + proportion * 143.0f) * juce::MathConstants<float>::pi / 180.0f;
        const float length = 198.0f * scale;
        const juce::Point<float> direction (std::cos (angle), std::sin (angle));
        const juce::Point<float> tip (pivot.x + direction.x * length,
                                      pivot.y + direction.y * length);
        const juce::Point<float> normal (-direction.y, direction.x);
        const float baseHalfWidth = 3.0f * scale;
        const float tipHalfWidth = 0.75f * scale;

        // Soft shadow gives the needle depth without the blunt look of a thick line.
        juce::Path shadow;
        shadow.startNewSubPath (pivot.x + normal.x * baseHalfWidth + 1.8f * scale,
                                pivot.y + normal.y * baseHalfWidth + 1.8f * scale);
        shadow.lineTo (tip.x + normal.x * tipHalfWidth + 1.8f * scale,
                       tip.y + normal.y * tipHalfWidth + 1.8f * scale);
        shadow.lineTo (tip.x - normal.x * tipHalfWidth + 1.8f * scale,
                       tip.y - normal.y * tipHalfWidth + 1.8f * scale);
        shadow.lineTo (pivot.x - normal.x * baseHalfWidth + 1.8f * scale,
                       pivot.y - normal.y * baseHalfWidth + 1.8f * scale);
        shadow.closeSubPath();
        g.setColour (juce::Colour (0x99000000));
        g.fillPath (shadow);

        juce::Path needle;
        needle.startNewSubPath (pivot.x + normal.x * baseHalfWidth,
                                pivot.y + normal.y * baseHalfWidth);
        needle.lineTo (tip.x + normal.x * tipHalfWidth,
                       tip.y + normal.y * tipHalfWidth);
        needle.lineTo (tip.x - normal.x * tipHalfWidth,
                       tip.y - normal.y * tipHalfWidth);
        needle.lineTo (pivot.x - normal.x * baseHalfWidth,
                       pivot.y - normal.y * baseHalfWidth);
        needle.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe4d4a7), pivot.x, pivot.y,
                                                 juce::Colour (0xff20282a), tip.x, tip.y, false));
        g.fillPath (needle);
        g.setColour (juce::Colour (0xfff7e8c2).withAlpha (0.9f));
        g.drawLine (pivot.x + normal.x * 0.7f * scale,
                    pivot.y + normal.y * 0.7f * scale,
                    tip.x + normal.x * 0.2f * scale,
                    tip.y + normal.y * 0.2f * scale,
                    0.7f * scale);

        // Machined pivot: dark socket, thin brass rim, and a small specular highlight.
        g.setColour (juce::Colour (0xff101314));
        g.fillEllipse (pivot.x - 8.0f * scale, pivot.y - 8.0f * scale,
                       16.0f * scale, 16.0f * scale);
        g.setColour (juce::Colour (0xffb8a071));
        g.drawEllipse (pivot.x - 8.0f * scale, pivot.y - 8.0f * scale,
                       16.0f * scale, 16.0f * scale, 1.3f * scale);
        g.setColour (juce::Colour (0xfff2dfb1));
        g.fillEllipse (pivot.x - 2.0f * scale, pivot.y - 2.0f * scale,
                       4.0f * scale, 4.0f * scale);
    };
    const float meterScale = juce::jmin (sx, sy);
    drawNeedle (audioProcessor.getInputMeterDb() + 18.0f,  { 332.0f * sx, 350.0f * sy }, meterScale);
    drawNeedle (audioProcessor.getOutputMeterDb() + 18.0f, { 1190.0f * sx, 350.0f * sy }, meterScale);

    // Keep the live bypass indicator visually in sync with its parameter.
    const bool bypass = audioProcessor.parameters.getRawParameterValue ("bypass")->load() > 0.5f;
    const juce::Rectangle<float> lamp (1316.0f * sx, 574.0f * sy, 40.0f * sx, 11.0f * sy);
    if (bypass)
    {
        g.setColour (juce::Colour (0x55ff9f1b));
        g.fillRoundedRectangle (lamp.expanded (4.0f * meterScale), 5.0f * meterScale);
        g.setColour (juce::Colour (0xffffa51f));
        g.fillRoundedRectangle (lamp, 4.0f * meterScale);
        g.setColour (juce::Colour (0xffffe8a2));
        g.fillRoundedRectangle (lamp.reduced (5.0f * meterScale, 3.0f * meterScale), 2.0f * meterScale);
    }
    else
    {
        // Opaque charcoal cover hides the reference artwork's always-lit lamp while off.
        g.setColour (juce::Colour (0xff262522));
        g.fillRoundedRectangle (lamp, 4.0f * meterScale);
        g.setColour (juce::Colour (0xff5b5140));
        g.drawRoundedRectangle (lamp, 4.0f * meterScale, 0.8f * meterScale);
    }
}

void VUMisterNAMAudioProcessorEditor::resized()
{
    const float sx = (float) getWidth() / 1522.0f;
    const float sy = (float) getHeight() / 833.0f;
    auto scaled = [sx, sy] (int x, int y, int w, int h) { return juce::Rectangle<int> ((int) (x * sx), (int) (y * sy), (int) (w * sx), (int) (h * sy)); };

    // Keep each interactive rotary inside its own faceplate well. The former 132 px
    // bounds extended beyond the artwork and made adjacent legends/controls appear crowded.
    inputSlider.setBounds (scaled (140, 493, 144, 144));
    mixSlider.setBounds (scaled (502, 493, 144, 144));
    outputSlider.setBounds (scaled (870, 493, 144, 144));
    // Restrict the invisible hit target to the illustrated switch body.
    // The BYPASS / OFF / ON legends remain unobstructed and no JUCE button chrome appears.
    bypassButton.setBounds (scaled (1305, 546, 70, 82));
    modelSelector.setBounds (scaled (220, 744, 555, 56));
    loadButton.setBounds (scaled (800, 744, 260, 56));
    // Status is available via tooltips; leave the bottom artwork row unobstructed.
    modelLabel.setBounds (0, 0, 0, 0);
    statusLabel.setBounds (0, 0, 0, 0);
}

void VUMisterNAMAudioProcessorEditor::timerCallback()
{
    // Refresh at 30 Hz so both meter needles follow incoming and processed audio.
    repaint();
}
