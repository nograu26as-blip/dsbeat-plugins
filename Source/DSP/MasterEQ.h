#pragma once

#include <juce_dsp/juce_dsp.h>

// 4-band parametric EQ: low shelf, 2x bell, high shelf.
// Processes a stereo AudioBlock in place via a ProcessorChain of IIR biquads.
class MasterEQ
{
public:
    MasterEQ();
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    struct Settings
    {
        float lowGainDb  = 0.0f, lowFreqHz  = 100.0f;
        float mid1GainDb = 0.0f, mid1FreqHz = 400.0f,  mid1Q = 1.0f;
        float mid2GainDb = 0.0f, mid2FreqHz = 2500.0f, mid2Q = 1.0f;
        float highGainDb = 0.0f, highFreqHz = 10000.0f;
        bool  bypassed   = false;
    };

    void updateCoefficients (const Settings& s);
    void process (juce::dsp::AudioBlock<float>& block);

    // Combined magnitude response (linear gain) at a frequency, for UI curve display.
    float getMagnitudeForFrequency (float freqHz) const;

private:
    double sampleRate = 44100.0;

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter> chain;

    enum { lowShelf = 0, mid1Bell = 1, mid2Bell = 2, highShelf = 3 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterEQ)
};
