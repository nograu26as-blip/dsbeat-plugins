#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class DSBeatMasterProcessor;

// Right-side meter bridge: LUFS, peak L/R, limiter GR, per-band GR, correlation.
class MeterBridge : public juce::Component, private juce::Timer
{
public:
    explicit MeterBridge (DSBeatMasterProcessor& proc);
    void paint (juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override { repaint(); }

    DSBeatMasterProcessor& processor;

    void drawBar (juce::Graphics& g, juce::Rectangle<float> area, float valueDb,
                  float minDb, float maxDb, const juce::String& label,
                  const juce::String& readout, bool showPeakMark = false,
                  float peakDb = -120.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterBridge)
};
