#include "MasterEQ.h"

MasterEQ::MasterEQ() = default;

void MasterEQ::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    chain.prepare (spec);
    chain.reset();
}

void MasterEQ::reset()
{
    chain.reset();
}

void MasterEQ::updateCoefficients (const Settings& s)
{
    auto sr = (float) sampleRate;

    *chain.get<lowShelf>().coefficients  = *Coeffs::makeLowShelf  (sr, s.lowFreqHz,  0.7071f, juce::Decibels::decibelsToGain (s.lowGainDb));
    *chain.get<mid1Bell>().coefficients  = *Coeffs::makePeakFilter (sr, s.mid1FreqHz, s.mid1Q,   juce::Decibels::decibelsToGain (s.mid1GainDb));
    *chain.get<mid2Bell>().coefficients  = *Coeffs::makePeakFilter (sr, s.mid2FreqHz, s.mid2Q,   juce::Decibels::decibelsToGain (s.mid2GainDb));
    *chain.get<highShelf>().coefficients = *Coeffs::makeHighShelf (sr, s.highFreqHz, 0.7071f, juce::Decibels::decibelsToGain (s.highGainDb));
}

void MasterEQ::process (juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    chain.process (ctx);
}

float MasterEQ::getMagnitudeForFrequency (float freqHz) const
{
    float mag = 1.0f;
    mag *= chain.get<lowShelf>().coefficients->getMagnitudeForFrequency (freqHz, sampleRate);
    mag *= chain.get<mid1Bell>().coefficients->getMagnitudeForFrequency (freqHz, sampleRate);
    mag *= chain.get<mid2Bell>().coefficients->getMagnitudeForFrequency (freqHz, sampleRate);
    mag *= chain.get<highShelf>().coefficients->getMagnitudeForFrequency (freqHz, sampleRate);
    return mag;
}
