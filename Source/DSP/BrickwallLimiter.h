#pragma once

#include <juce_dsp/juce_dsp.h>
#include <atomic>

// Lookahead brickwall limiter.
// A short delay line lets the gain-reduction envelope react instantly to
// peaks: the envelope is computed from the undelayed input while the gain
// is applied to the delayed signal. A final hard clamp guarantees the
// ceiling is never exceeded.
class BrickwallLimiter
{
public:
    BrickwallLimiter();
    void prepare (double sampleRate, int numChannels, int maxBlockSize);
    void reset();

    struct Settings
    {
        float ceilingDb = -1.0f;   // -12 .. 0
        float releaseMs = 50.0f;   // 10 .. 500
        bool  bypassed  = false;
    };

    void process (juce::AudioBuffer<float>& buffer, const Settings& s);

    float getGainReductionDb() const noexcept { return grDb.load (std::memory_order_relaxed); }

private:
    double sampleRate = 44100.0;
    int numChannels = 2;

    // Lookahead delay line (per channel).
    juce::AudioBuffer<float> delayLine;
    int delaySamples = 0;
    int writePos = 0;

    float env = 1.0f; // current linear gain
    std::atomic<float> grDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrickwallLimiter)
};
