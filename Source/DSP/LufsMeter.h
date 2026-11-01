#pragma once

#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

// Loudness / level analysis tap (runs on the processed output).
//  - K-weighting: high shelf +4 dB @ 1681 Hz (Q 0.7071) followed by
//    2nd-order high-pass @ 38.1 Hz (Q 0.5) — BS.1770-style approximation.
//  - Momentary loudness: 400 ms sliding window, -0.691 offset.
//  - Per-channel true-ish peak with hold + decay.
//  - Stereo correlation over the current block, smoothed.
class LufsMeter
{
public:
    LufsMeter();
    void prepare (double sampleRate, int numChannels, int maxBlockSize);
    void reset();

    // Analyse a stereo (or mono) buffer. Call once per processBlock.
    void process (const juce::AudioBuffer<float>& buffer);

    float getMomentaryLUFS() const noexcept { return momentaryLufs.load (std::memory_order_relaxed); }
    float getPeakDb (int channel) const noexcept;
    float getCorrelation() const noexcept { return correlation.load (std::memory_order_relaxed); }

private:
    double sampleRate = 44100.0;
    int numChannels = 2;

    juce::dsp::IIR::Filter<float> preFilter[2];
    juce::dsp::IIR::Filter<float> rlbFilter[2];

    // 400 ms sliding energy window.
    std::vector<float> ring;
    int ringPos = 0;
    int ringSize = 1;
    double ringSum = 0.0;

    float peakLin[2] = { 0.0f, 0.0f };
    std::atomic<float> peakHoldDb[2];
    int   peakHoldCounter[2] = { 0, 0 };

    std::atomic<float> momentaryLufs { -70.0f };
    std::atomic<float> correlation { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeter)
};
