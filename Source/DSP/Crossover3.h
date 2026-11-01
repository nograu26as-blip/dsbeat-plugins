#pragma once

#include <juce_dsp/juce_dsp.h>

// Splits a stereo signal into 3 bands (low / mid / high) using 4th-order
// Linkwitz-Riley crossovers. Shared by the multiband compressor and the
// stereo imager so both stay phase-coherent.
class Crossover3
{
public:
    Crossover3();
    void prepare (const juce::dsp::ProcessSpec& spec, int maxBlockSize);
    void reset();
    void setFrequencies (float lowHz, float highHz);

    // Splits `input` into low/mid/high. Buffers must be pre-sized to
    // (numChannels x numSamples). Input is not modified.
    void split (const juce::AudioBuffer<float>& input,
                juce::AudioBuffer<float>& low,
                juce::AudioBuffer<float>& mid,
                juce::AudioBuffer<float>& high);

private:
    juce::dsp::LinkwitzRileyFilter<float> lp1, hp1, lp2, hp2;
    juce::AudioBuffer<float> tmp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Crossover3)
};
