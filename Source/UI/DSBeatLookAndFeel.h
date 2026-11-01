#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// DS BEAT dark chrome look: near-black panels, silver/chrome accents.
class DSBeatLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DSBeatLookAndFeel();

    // Palette
    static const juce::Colour bg;          // window background
    static const juce::Colour panel;       // panel background
    static const juce::Colour panelEdge;   // panel border
    static const juce::Colour chromeHi;    // chrome highlight
    static const juce::Colour chromeLo;    // chrome shadow
    static const juce::Colour text;        // main text
    static const juce::Colour textDim;     // dim text
    static const juce::Colour accentRed;   // gain reduction / clip
    static const juce::Colour accentGreen; // ok / active
    static const juce::Colour accentBlue;  // meter fill

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawTabButton (juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver,
                        bool isMouseDown) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    juce::Font getLabelFont (juce::Label& label) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSBeatLookAndFeel)
};
