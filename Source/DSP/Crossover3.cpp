#include "Crossover3.h"

Crossover3::Crossover3() = default;

void Crossover3::prepare (const juce::dsp::ProcessSpec& spec, int maxBlockSize)
{
    lp1.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    hp1.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    lp2.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    hp2.setType (juce::dsp::LinkwitzRileyFilterType::highpass);

    lp1.prepare (spec); hp1.prepare (spec);
    lp2.prepare (spec); hp2.prepare (spec);

    tmp.setSize ((int) spec.numChannels, maxBlockSize);

    lp1.reset(); hp1.reset(); lp2.reset(); hp2.reset();
}

void Crossover3::reset()
{
    lp1.reset(); hp1.reset(); lp2.reset(); hp2.reset();
}

void Crossover3::setFrequencies (float lowHz, float highHz)
{
    lp1.setCutoffFrequency (lowHz);
    hp1.setCutoffFrequency (lowHz);
    lp2.setCutoffFrequency (highHz);
    hp2.setCutoffFrequency (highHz);
}

void Crossover3::split (const juce::AudioBuffer<float>& input,
                        juce::AudioBuffer<float>& low,
                        juce::AudioBuffer<float>& mid,
                        juce::AudioBuffer<float>& high)
{
    const int n = input.getNumSamples();

    for (int ch = 0; ch < input.getNumChannels(); ++ch)
    {
        low.copyFrom  (ch, 0, input, ch, 0, n);
        tmp.copyFrom  (ch, 0, input, ch, 0, n);
    }

    juce::dsp::AudioBlock<float> lowBlock (low);
    lp1.process (juce::dsp::ProcessContextReplacing<float> (lowBlock));

    juce::dsp::AudioBlock<float> tmpBlock (tmp);
    hp1.process (juce::dsp::ProcessContextReplacing<float> (tmpBlock));

    for (int ch = 0; ch < input.getNumChannels(); ++ch)
    {
        mid.copyFrom  (ch, 0, tmp, ch, 0, n);
        high.copyFrom (ch, 0, tmp, ch, 0, n);
    }

    juce::dsp::AudioBlock<float> midBlock (mid);
    lp2.process (juce::dsp::ProcessContextReplacing<float> (midBlock));

    juce::dsp::AudioBlock<float> highBlock (high);
    hp2.process (juce::dsp::ProcessContextReplacing<float> (highBlock));
}
