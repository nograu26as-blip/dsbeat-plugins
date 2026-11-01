#include "MeterBridge.h"
#include "DSBeatLookAndFeel.h"
#include "../PluginProcessor.h"

MeterBridge::MeterBridge (DSBeatMasterProcessor& proc)
    : processor (proc)
{
    startTimerHz (30);
}

void MeterBridge::drawBar (juce::Graphics& g, juce::Rectangle<float> area, float valueDb,
                           float minDb, float maxDb, const juce::String& label,
                           const juce::String& readout, bool showPeakMark, float peakDb)
{
    g.setColour (DSBeatLookAndFeel::textDim);
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText (label, area.getX(), area.getY() - 14, area.getWidth(), 12,
                juce::Justification::centredLeft, false);
    g.setColour (DSBeatLookAndFeel::text);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText (readout, area.getX(), area.getY() - 14, area.getWidth(), 12,
                juce::Justification::centredRight, false);

    g.setColour (juce::Colour (0xff0d0d10));
    g.fillRoundedRectangle (area, 4.0f);

    const float t = juce::jlimit (0.0f, 1.0f, (valueDb - minDb) / (maxDb - minDb));
    const float w = area.getWidth() * t;

    // Gradient: green -> amber -> red as it rises.
    juce::Colour c = t < 0.7f ? DSBeatLookAndFeel::accentGreen
                   : t < 0.9f ? juce::Colour (0xfffbbf24)
                              : DSBeatLookAndFeel::accentRed;
    g.setColour (c);
    if (w > 1.0f)
        g.fillRoundedRectangle (area.getX(), area.getY(), w, area.getHeight(), 4.0f);

    if (showPeakMark)
    {
        const float pt = juce::jlimit (0.0f, 1.0f, (peakDb - minDb) / (maxDb - minDb));
        g.setColour (DSBeatLookAndFeel::chromeHi);
        g.fillRect (area.getX() + area.getWidth() * pt - 1.0f, area.getY(), 2.0f, area.getHeight());
    }

    g.setColour (DSBeatLookAndFeel::panelEdge);
    g.drawRoundedRectangle (area, 4.0f, 1.0f);
}

void MeterBridge::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (DSBeatLookAndFeel::panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (DSBeatLookAndFeel::panelEdge);
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    float y = bounds.getY() + 26.0f;
    const float x = bounds.getX() + 12.0f;
    const float w = bounds.getWidth() - 24.0f;
    constexpr float barH = 14.0f;
    constexpr float gap  = 30.0f;

    // LUFS (momentary)
    {
        const float lufs = processor.getMomentaryLUFS();
        drawBar (g, { x, y, w, barH }, lufs, -60.0f, 0.0f, "LOUDNESS",
                 juce::String (lufs, 1) + " LUFS");
        y += gap;
    }

    // Peak L / R
    for (int ch = 0; ch < 2; ++ch)
    {
        const float pk = processor.getOutputPeakDb (ch);
        drawBar (g, { x, y, w, barH }, pk, -60.0f, 6.0f,
                 ch == 0 ? "PEAK L" : "PEAK R",
                 juce::String (pk, 1) + " dB", true, pk);
        y += gap;
    }

    // Input peak (dimmer, for reference)
    for (int ch = 0; ch < 2; ++ch)
    {
        const float pk = processor.getInputPeakDb (ch);
        drawBar (g, { x, y, w, 8.0f }, pk, -60.0f, 6.0f,
                 ch == 0 ? "IN L" : "IN R",
                 juce::String (pk, 1) + " dB");
        y += 24.0f;
    }

    // Limiter GR
    {
        const float gr = processor.getLimiterGainReductionDb(); // <= 0
        drawBar (g, { x, y, w, barH }, gr, -24.0f, 0.0f, "LIMITER GR",
                 juce::String (-gr, 1) + " dB");
        y += gap;
    }

    // Per-band GR
    {
        const char* names[3] = { "LOW", "MID", "HIGH" };
        const float bw = (w - 16.0f) / 3.0f;
        for (int b = 0; b < 3; ++b)
        {
            const float gr = processor.getBandGainReductionDb (b);
            drawBar (g, { x + b * (bw + 8.0f), y, bw, barH }, gr, -24.0f, 0.0f,
                     juce::String ("GR ") + names[b], juce::String (-gr, 1));
        }
        y += gap;
    }

    // Correlation
    {
        const float corr = processor.getCorrelation();
        g.setColour (DSBeatLookAndFeel::textDim);
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.drawText ("CORRELATION", x, y - 14, w, 12, juce::Justification::centredLeft, false);
        g.setColour (DSBeatLookAndFeel::text);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (juce::String (corr, 2), x, y - 14, w, 12, juce::Justification::centredRight, false);

        juce::Rectangle<float> area (x, y, w, barH);
        g.setColour (juce::Colour (0xff0d0d10));
        g.fillRoundedRectangle (area, 4.0f);

        const float cx = area.getCentreX();
        const float half = area.getWidth() * 0.5f;
        // centre line
        g.setColour (DSBeatLookAndFeel::panelEdge);
        g.drawVerticalLine ((int) cx, area.getY(), area.getBottom());

        const float fillW = half * std::abs (corr);
        juce::Colour c = corr >= 0 ? DSBeatLookAndFeel::accentGreen : DSBeatLookAndFeel::accentRed;
        g.setColour (c);
        if (corr >= 0)
            g.fillRect (cx, area.getY(), fillW, area.getHeight());
        else
            g.fillRect (cx - fillW, area.getY(), fillW, area.getHeight());

        g.setColour (DSBeatLookAndFeel::panelEdge);
        g.drawRoundedRectangle (area, 4.0f, 1.0f);
    }
}
