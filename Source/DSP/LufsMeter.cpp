#include "LufsMeter.h"

LufsMeter::LufsMeter() = default;

void LufsMeter::prepare (double sr, int nch, int /*maxBlockSize*/)
{
    sampleRate  = sr;
    numChannels = juce::jmin (nch, 2);

    juce::dsp::ProcessSpec spec { sr, 512, (juce::uint32) numChannels };

    for (int ch = 0; ch < numChannels; ++ch)
    {
        preFilter[ch].prepare (spec);
        rlbFilter[ch].prepare (spec);

        // Stage 1: pre-filter, high shelf +3.99 dB @ 1681.97 Hz, Q 0.7071
        *preFilter[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sr, 1681.97f, 0.7071f, juce::Decibels::decibelsToGain (3.99f));

        // Stage 2: RLB high-pass @ 38.13 Hz, Q 0.5003
        *rlbFilter[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
            sr, 38.13f, 0.5003f);

        preFilter[ch].reset();
        rlbFilter[ch].reset();
    }

    ringSize = juce::jmax (1, (int) std::round (0.4 * sr)); // 400 ms
    ring.assign ((size_t) ringSize, 0.0f);
    ringPos = 0;
    ringSum = 0.0;

    reset();
}

void LufsMeter::reset()
{
    std::fill (ring.begin(), ring.end(), 0.0f);
    ringPos = 0;
    ringSum = 0.0;
    for (int ch = 0; ch < 2; ++ch)
    {
        peakLin[ch] = 0.0f;
        peakHoldDb[ch].store (-120.0f, std::memory_order_relaxed);
        peakHoldCounter[ch] = 0;
        if (ch < numChannels)
        {
            preFilter[ch].reset();
            rlbFilter[ch].reset();
        }
    }
    momentaryLufs.store (-70.0f, std::memory_order_relaxed);
    correlation.store (0.0f, std::memory_order_relaxed);
}

void LufsMeter::process (const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    const int nch = juce::jmin (buffer.getNumChannels(), numChannels);

    double sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;

    for (int i = 0; i < n; ++i)
    {
        float chE[2] = { 0.0f, 0.0f };

        for (int ch = 0; ch < nch; ++ch)
        {
            const float x = buffer.getSample (ch, i);

            // Unweighted peak tracking (fast attack, slow release).
            const float ax = std::abs (x);
            if (ax > peakLin[ch])
                peakLin[ch] = ax;
            else
                peakLin[ch] *= 0.99990f; // ~slow decay

            // K-weighted energy.
            float y = preFilter[ch].processSample (x);
            y = rlbFilter[ch].processSample (y);
            chE[ch] = y * y;
        }

        const float e = (nch == 2) ? 0.5f * (chE[0] + chE[1]) : chE[0];

        ringSum += (double) e - (double) ring[(size_t) ringPos];
        ring[(size_t) ringPos] = e;
        if (++ringPos >= ringSize)
            ringPos = 0;

        if (nch == 2)
        {
            const float l = buffer.getSample (0, i);
            const float r = buffer.getSample (1, i);
            sumLR += (double) l * r;
            sumL2 += (double) l * l;
            sumR2 += (double) r * r;
        }
    }

    // Momentary loudness (400 ms window).
    const double meanSquare = ringSum / (double) ringSize;
    const float lufs = (meanSquare > 1.0e-12)
        ? (float) (-0.691 + 10.0 * std::log10 (meanSquare))
        : -70.0f;
    momentaryLufs.store (juce::jlimit (-70.0f, 6.0f, lufs), std::memory_order_relaxed);

    // Peak hold (hold ~1.5 s at 30 fps UI, decay after).
    for (int ch = 0; ch < nch; ++ch)
    {
        const float peakDb = juce::Decibels::gainToDecibels (peakLin[ch], -120.0f);
        const float held = peakHoldDb[ch].load (std::memory_order_relaxed);
        if (peakDb >= held)
        {
            peakHoldDb[ch].store (peakDb, std::memory_order_relaxed);
            peakHoldCounter[ch] = 45;
        }
        else if (--peakHoldCounter[ch] <= 0)
        {
            peakHoldDb[ch].store (juce::jmax (-120.0f, held - 1.5f), std::memory_order_relaxed);
            peakHoldCounter[ch] = 2;
        }
    }

    // Correlation.
    if (nch == 2 && sumL2 > 1.0e-12 && sumR2 > 1.0e-12)
    {
        const float c = (float) (sumLR / std::sqrt (sumL2 * sumR2));
        const float prev = correlation.load (std::memory_order_relaxed);
        correlation.store (prev * 0.7f + juce::jlimit (-1.0f, 1.0f, c) * 0.3f,
                           std::memory_order_relaxed);
    }
}

float LufsMeter::getPeakDb (int channel) const noexcept
{
    if (channel < 0 || channel > 1)
        return -120.0f;
    return peakHoldDb[channel].load (std::memory_order_relaxed);
}
