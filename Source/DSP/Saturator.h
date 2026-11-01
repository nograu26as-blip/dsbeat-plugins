#pragma once

#include <juce_dsp/juce_dsp.h>

// Tube-style harmonic saturator / exciter.
// Asymmetric tanh waveshaper adds warm even + odd harmonics, with
// drive amount and dry/wet mix. Includes a gentle output trim so the
// stage stays level-matched as drive increases.
class Saturator
{
public:
    Saturator() = default;

    struct Settings
    {
        float drive = 0.0f;  // 0..1
        float mix   = 1.0f;  // 0..1 dry/wet
        bool  bypassed = false;
    };

    void process (juce::AudioBuffer<float>& buffer, const Settings& s) const;

private:
    static float shapeSample (float x, float drive) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Saturator)
};
