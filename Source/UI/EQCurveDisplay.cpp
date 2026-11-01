#include "EQCurveDisplay.h"
#include "DSBeatLookAndFeel.h"

EQCurveDisplay::EQCurveDisplay()
{
    startTimerHz (20);
}

void EQCurveDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff0d0d10));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (DSBeatLookAndFeel::panelEdge);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    auto plot = bounds.reduced (8.0f, 6.0f);
    const float minDb = -18.0f, maxDb = 18.0f;

    auto freqToX = [&] (float f)
    {
        const float t = std::log (f / 20.0f) / std::log (20000.0f / 20.0f);
        return plot.getX() + t * plot.getWidth();
    };
    auto dbToY = [&] (float db)
    {
        const float t = (db - minDb) / (maxDb - minDb);
        return plot.getBottom() - t * plot.getHeight();
    };

    // Grid: frequency lines.
    g.setColour (juce::Colour (0xff222228));
    const float gridFreqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    g.setFont (juce::Font (9.0f));
    for (float f : gridFreqs)
    {
        const float x = freqToX (f);
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        juce::String label = f >= 1000 ? juce::String (f / 1000.0f, 0) + "k"
                                       : juce::String ((int) f);
        g.setColour (juce::Colour (0xff55555e));
        g.drawText (label, (int) x - 20, (int) plot.getBottom() - 12, 40, 12,
                    juce::Justification::centred, false);
        g.setColour (juce::Colour (0xff222228));
    }

    // Grid: dB lines.
    for (float db = -12.0f; db <= 12.0f; db += 6.0f)
    {
        const float y = dbToY (db);
        g.setColour (db == 0.0f ? juce::Colour (0xff3a3a44) : juce::Colour (0xff222228));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
    }

    if (eq == nullptr)
        return;

    // Curve.
    juce::Path curve;
    bool started = false;
    for (int i = 0; i < numPoints; ++i)
    {
        const float t = (float) i / (float) (numPoints - 1);
        const float f = 20.0f * std::pow (20000.0f / 20.0f, t);
        const float magDb = juce::Decibels::gainToDecibels (eq->getMagnitudeForFrequency (f));
        const float x = freqToX (f);
        const float y = dbToY (juce::jlimit (minDb, maxDb, magDb));

        if (! started) { curve.startNewSubPath (x, y); started = true; }
        else           { curve.lineTo (x, y); }
    }

    // Fill under curve.
    juce::Path fill = curve;
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();
    juce::ColourGradient fillGrad (juce::Colour (0x402a2a30), plot.getX(), plot.getY(),
                                   juce::Colour (0x00101012), plot.getX(), plot.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (fill);

    // Chrome stroke.
    juce::ColourGradient chrome (DSBeatLookAndFeel::chromeHi, plot.getX(), 0,
                                 DSBeatLookAndFeel::chromeLo, plot.getRight(), 0, false);
    g.setGradientFill (chrome);
    g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}
