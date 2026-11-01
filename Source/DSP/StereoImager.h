#pragma once

#include "Crossover3.h"

// Per-band stereo width control (low / mid / high).
// Each band is converted to mid/side, the side signal is scaled by the
// width factor (0 = mono, 1 = unchanged, 2 = extra wide), then converted back.
class StereoImager
{
public:
    StereoImager();
    void prepare (const juce::dsp::ProcessSpec& spec, int maxBlockSize);
    void reset();

    struct Settings
    {
        float xoverLowHz = 200.0f, xoverHighHz = 3000.0f;
        float widthLow = 1.0f, widthMid = 1.0f, widthHigh = 1.0f; // 0..2
        bool  bypassed = false;
    };

    void process (juce::AudioBuffer<float>& buffer, const Settings& s);

private:
    Crossover3 xo;
    juce::AudioBuffer<float> lowBuf, midBuf, highBuf;

    static void applyWidth (juce::AudioBuffer<float>& band, float width);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoImager)
};
