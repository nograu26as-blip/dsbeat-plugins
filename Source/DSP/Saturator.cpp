#include "Saturator.h"

float Saturator::shapeSample (float x, float drive) noexcept
{
    // Pre-gain into the shaper: up to ~21x at full drive.
    const float k = 1.0f + drive * 20.0f;
    const float driven = x * k;

    // Symmetric tanh core (odd harmonics) ...
    const float t = std::tanh (driven);

    // ... plus an asymmetric term for tube-like even harmonics.
    // t*t is always positive -> adds 2nd-harmonic-style asymmetry.
    const float asym = 0.18f * drive * t * t;

    float y = t + asym;

    // Level-compensate so drive doesn't just get louder.
    const float trim = 1.0f / (1.0f + drive * 6.0f);
    return y * trim;
}

void Saturator::process (juce::AudioBuffer<float>& buffer, const Settings& s) const
{
    if (s.drive <= 0.0001f || s.mix <= 0.0001f)
        return;

    const float mix = juce::jlimit (0.0f, 1.0f, s.mix);
    const float drive = juce::jlimit (0.0f, 1.0f, s.drive);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch, 0);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            const float dry = d[i];
            const float wet = shapeSample (dry, drive);
            d[i] = dry + (wet - dry) * mix;
        }
    }
}
