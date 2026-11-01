#include "DSBeatLookAndFeel.h"

const juce::Colour DSBeatLookAndFeel::bg         (0xff0a0a0c);
const juce::Colour DSBeatLookAndFeel::panel      (0xff141417);
const juce::Colour DSBeatLookAndFeel::panelEdge  (0xff2b2b32);
const juce::Colour DSBeatLookAndFeel::chromeHi   (0xfff4f4f7);
const juce::Colour DSBeatLookAndFeel::chromeLo   (0xff7c7c86);
const juce::Colour DSBeatLookAndFeel::text       (0xffdcdce0);
const juce::Colour DSBeatLookAndFeel::textDim    (0xff8b8b94);
const juce::Colour DSBeatLookAndFeel::accentRed  (0xffe05252);
const juce::Colour DSBeatLookAndFeel::accentGreen(0xff4ade80);
const juce::Colour DSBeatLookAndFeel::accentBlue (0xff7dd3fc);

DSBeatLookAndFeel::DSBeatLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0e0e11));
    setColour (juce::Slider::textBoxOutlineColourId, panelEdge);
    setColour (juce::ToggleButton::textColourId, text);
    setColour (juce::TabbedComponent::backgroundColourId, bg);
    setColour (juce::TabbedComponent::outlineColourId, panelEdge);
}

juce::Font DSBeatLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (13.0f, juce::Font::bold);
}

void DSBeatLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider&)
{
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float radius = juce::jmin (width, height) * 0.5f - 6.0f;

    // Body: dark disc with subtle rim.
    juce::ColourGradient bodyGrad (juce::Colour (0xff1d1d22), cx, cy - radius,
                                   juce::Colour (0xff0c0c0f), cx, cy + radius, false);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (panelEdge);
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // Track arc.
    juce::Path track;
    track.addCentredArc (cx, cy, radius - 4.0f, radius - 4.0f, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff2e2e36));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));

    // Value arc: chrome gradient.
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    if (sliderPosProportional > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (cx, cy, radius - 4.0f, radius - 4.0f, 0.0f,
                             rotaryStartAngle, angle, true);
        juce::ColourGradient chrome (chromeHi, cx - radius, cy,
                                     chromeLo, cx + radius, cy, false);
        g.setGradientFill (chrome);
        g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Pointer.
    const float pr = radius - 10.0f;
    const float px = cx + std::sin (angle) * pr;
    const float py = cy - std::cos (angle) * pr;
    g.setColour (chromeHi);
    g.drawLine (cx, cy, px, py, 2.5f);
    g.fillEllipse (px - 2.5f, py - 2.5f, 5.0f, 5.0f);
}

void DSBeatLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = button.getToggleState();

    juce::ColourGradient grad (
        on ? juce::Colour (0xff3a3a44) : juce::Colour (0xff17171b),
        bounds.getX(), bounds.getY(),
        on ? juce::Colour (0xff232329) : juce::Colour (0xff0e0e11),
        bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (on ? chromeHi : panelEdge);
    g.drawRoundedRectangle (bounds, 6.0f, on ? 1.5f : 1.0f);

    if (on)
    {
        g.setColour (accentGreen);
        g.fillEllipse (bounds.getX() + 6.0f, bounds.getCentreY() - 3.0f, 6.0f, 6.0f);
    }

    g.setColour (on ? text : textDim);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    juce::Rectangle<float> textBounds = bounds.reduced (16.0f, 0.0f);
    if (on)
        textBounds.removeFromLeft (8.0f);
    g.drawText (button.getButtonText(), textBounds,
                juce::Justification::centredLeft, false);

    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void DSBeatLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                       bool isMouseOver, bool isMouseDown)
{
    auto bounds = button.getActiveArea().toFloat();
    const bool active = button.getToggleState();

    g.setColour (active ? panel : juce::Colour (0xff0e0e11));
    g.fillRect (bounds);

    if (active)
    {
        g.setColour (chromeHi);
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 2.5f);
    }

    g.setColour (active ? text : textDim);
    g.setFont (juce::Font (13.0f, active ? juce::Font::bold : juce::Font::plain));
    g.drawText (button.getButtonText().toUpperCase(), bounds,
                juce::Justification::centred, false);

    juce::ignoreUnused (isMouseOver, isMouseDown);
}

void DSBeatLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float minSliderPos, float maxSliderPos,
                                          const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos, style, slider);
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    g.setColour (juce::Colour (0xff1a1a1f));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (accentBlue);
    g.fillRoundedRectangle (juce::Rectangle<float> (bounds.getX(), bounds.getY(),
                                                    sliderPos - bounds.getX(), bounds.getHeight()), 3.0f);
}
