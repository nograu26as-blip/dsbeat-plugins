#pragma once

#include "Crossover3.h"
#include <atomic>

// Single-band feedforward compressor with peak detector and
// attack/release ballistics. Operates on a stereo band buffer.
class BandCompressor
{
public:
    BandCompressor() = default;
    void prepare (double sampleRate);
    void reset();

    void process (juce::AudioBuffer<float>& bandBuffer,
                  float threshDb, float ratio,
                  float attackMs, float releaseMs,
                  float makeupDb);

    float getGainReductionDb() const noexcept { return grDb.load (std::memory_order_relaxed); }

private:
    double sampleRate = 44100.0;
    float env = 0.0f;
    std::atomic<float> grDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandCompressor)
};

// 3-band multiband compressor: LR crossover -> per-band compressor -> sum.
class MultibandComp
{
public:
    MultibandComp();
    void prepare (const juce::dsp::ProcessSpec& spec, int maxBlockSize);
    void reset();

    struct BandSettings
    {
        float threshDb = -24.0f, ratio = 2.0f;
        float attackMs = 10.0f,  releaseMs = 100.0f;
        float makeupDb = 0.0f;
    };

    struct Settings
    {
        float xoverLowHz = 200.0f, xoverHighHz = 3000.0f;
        BandSettings bands[3]; // 0=low, 1=mid, 2=high
        bool bypassed = false;
    };

    // Processes stereo buffer in place.
    void process (juce::AudioBuffer<float>& buffer, const Settings& s);

    float getBandGainReductionDb (int band) const noexcept;

private:
    Crossover3 xo;
    BandCompressor comps[3];
    juce::AudioBuffer<float> lowBuf, midBuf, highBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandComp)
};
