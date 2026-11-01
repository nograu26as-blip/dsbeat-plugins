#include "MultibandComp.h"

//==============================================================================
void BandCompressor::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void BandCompressor::reset()
{
    env = 0.0f;
    grDb.store (0.0f, std::memory_order_relaxed);
}

void BandCompressor::process (juce::AudioBuffer<float>& bandBuffer,
                              float threshDb, float ratio,
                              float attackMs, float releaseMs,
                              float makeupDb)
{
    const float attackCoef  = std::exp (-1.0f / (0.001f * juce::jmax (attackMs,  0.05f) * (float) sampleRate));
    const float releaseCoef = std::exp (-1.0f / (0.001f * juce::jmax (releaseMs, 1.0f)  * (float) sampleRate));
    const float threshLin   = juce::Decibels::decibelsToGain (threshDb);
    const float makeupLin   = juce::Decibels::decibelsToGain (makeupDb);
    const float invRatio    = 1.0f - 1.0f / juce::jmax (ratio, 1.0f);

    const int numCh = bandBuffer.getNumChannels();
    const int n     = bandBuffer.getNumSamples();

    float maxGrDb = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (bandBuffer.getSample (ch, i)));

        const float coef = (peak > env) ? attackCoef : releaseCoef;
        env = coef * env + (1.0f - coef) * peak;

        float grDbNow = 0.0f;
        if (env > threshLin && env > 1.0e-6f)
        {
            const float overDb = juce::Decibels::gainToDecibels (env) - threshDb;
            grDbNow = -overDb * invRatio;
        }

        maxGrDb = juce::jmin (maxGrDb, grDbNow);

        const float g = juce::Decibels::decibelsToGain (grDbNow) * makeupLin;
        for (int ch = 0; ch < numCh; ++ch)
            bandBuffer.setSample (ch, i, bandBuffer.getSample (ch, i) * g);
    }

    grDb.store (maxGrDb, std::memory_order_relaxed);
}

//==============================================================================
MultibandComp::MultibandComp() = default;

void MultibandComp::prepare (const juce::dsp::ProcessSpec& spec, int maxBlockSize)
{
    xo.prepare (spec, maxBlockSize);

    const int nch = (int) spec.numChannels;
    lowBuf.setSize  (nch, maxBlockSize);
    midBuf.setSize  (nch, maxBlockSize);
    highBuf.setSize (nch, maxBlockSize);

    for (auto& c : comps)
        c.prepare (spec.sampleRate);

    reset();
}

void MultibandComp::reset()
{
    xo.reset();
    for (auto& c : comps)
        c.reset();
}

void MultibandComp::process (juce::AudioBuffer<float>& buffer, const Settings& s)
{
    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    xo.setFrequencies (s.xoverLowHz, s.xoverHighHz);
    xo.split (buffer, lowBuf, midBuf, highBuf);

    juce::AudioBuffer<float>* bands[3] = { &lowBuf, &midBuf, &highBuf };

    for (int b = 0; b < 3; ++b)
    {
        const auto& bs = s.bands[b];
        comps[b].process (*bands[b], bs.threshDb, bs.ratio, bs.attackMs, bs.releaseMs, bs.makeupDb);
    }

    // Sum bands back into buffer (LR crossovers sum flat).
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* dst = buffer.getWritePointer (ch, 0);
        auto* l = lowBuf.getReadPointer  (ch, 0);
        auto* m = midBuf.getReadPointer  (ch, 0);
        auto* h = highBuf.getReadPointer (ch, 0);
        for (int i = 0; i < n; ++i)
            dst[i] = l[i] + m[i] + h[i];
    }
}

float MultibandComp::getBandGainReductionDb (int band) const noexcept
{
    return comps[juce::jlimit (0, 2, band)].getGainReductionDb();
}
