#include "BrickwallLimiter.h"

BrickwallLimiter::BrickwallLimiter() = default;

void BrickwallLimiter::prepare (double sr, int nch, int maxBlockSize)
{
    sampleRate  = sr;
    numChannels = nch;

    // 1.5 ms lookahead.
    delaySamples = juce::jmax (1, (int) std::round (0.0015 * sr));

    // Delay line must hold lookahead + one full block for the read offset.
    delayLine.setSize (nch, delaySamples + maxBlockSize + 8);
    delayLine.clear();

    reset();
}

void BrickwallLimiter::reset()
{
    delayLine.clear();
    writePos = 0;
    env = 1.0f;
    grDb.store (0.0f, std::memory_order_relaxed);
}

void BrickwallLimiter::process (juce::AudioBuffer<float>& buffer, const Settings& s)
{
    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    const float ceilLin   = juce::Decibels::decibelsToGain (s.ceilingDb);
    const float releaseMs = juce::jmax (s.releaseMs, 1.0f);
    const float relCoef   = std::exp (-1.0f / (0.001f * releaseMs * (float) sampleRate));
    const int   lineSize  = delayLine.getNumSamples();

    float maxGrDb = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        // Peak across channels on the undelayed input.
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float x = buffer.getSample (ch, i);
            peak = juce::jmax (peak, std::abs (x));
            delayLine.setSample (ch, writePos, x);
        }

        // Instant attack (lookahead covers it), exponential release.
        const float target = (peak > ceilLin && peak > 1.0e-7f) ? (ceilLin / peak) : 1.0f;
        if (target < env)
            env = target;
        else
            env = relCoef * env + (1.0f - relCoef) * target;

        const float grDbNow = juce::Decibels::gainToDecibels (env);
        maxGrDb = juce::jmin (maxGrDb, grDbNow);

        // Read the delayed signal and apply the envelope.
        int readPos = writePos - delaySamples;
        if (readPos < 0)
            readPos += lineSize;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float y = delayLine.getSample (ch, readPos) * env;
            // Safety clamp: brickwall guarantee.
            y = juce::jlimit (-ceilLin, ceilLin, y);
            buffer.setSample (ch, i, y);
        }

        if (++writePos >= lineSize)
            writePos = 0;
    }

    grDb.store (maxGrDb, std::memory_order_relaxed);
}
