#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/MasterEQ.h"
#include "DSP/MultibandComp.h"
#include "DSP/StereoImager.h"
#include "DSP/Saturator.h"
#include "DSP/BrickwallLimiter.h"
#include "DSP/LufsMeter.h"

// Parameter IDs
namespace IDs
{
    constexpr const char* masterBypass = "masterBypass";

    constexpr const char* inGain   = "inGain";
    constexpr const char* autoGain = "autoGain";

    constexpr const char* eqBypass = "eqBypass";
    constexpr const char* lowGain  = "lowGain";
    constexpr const char* lowFreq  = "lowFreq";
    constexpr const char* mid1Gain = "mid1Gain";
    constexpr const char* mid1Freq = "mid1Freq";
    constexpr const char* mid1Q    = "mid1Q";
    constexpr const char* mid2Gain = "mid2Gain";
    constexpr const char* mid2Freq = "mid2Freq";
    constexpr const char* mid2Q    = "mid2Q";
    constexpr const char* highGain = "highGain";
    constexpr const char* highFreq = "highFreq";

    constexpr const char* mbBypass  = "mbBypass";
    constexpr const char* xoverLow  = "xoverLow";
    constexpr const char* xoverHigh = "xoverHigh";
    // per band: "thresh0".."thresh2", "ratio0".., "attack0".., "release0".., "makeup0"..

    constexpr const char* imagerBypass = "imagerBypass";
    constexpr const char* widthLow  = "widthLow";
    constexpr const char* widthMid  = "widthMid";
    constexpr const char* widthHigh = "widthHigh";

    constexpr const char* satBypass = "satBypass";
    constexpr const char* satDrive  = "satDrive";
    constexpr const char* satMix    = "satMix";

    constexpr const char* limBypass  = "limBypass";
    constexpr const char* ceiling    = "ceiling";
    constexpr const char* limRelease = "limRelease";

    constexpr const char* outGain = "outGain";

    inline juce::String bandParam (const char* base, int band)
    {
        return juce::String (base) + juce::String (band);
    }
}

//==============================================================================
class DSBeatMasterProcessor : public juce::AudioProcessor
{
public:
    DSBeatMasterProcessor();
    ~DSBeatMasterProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DSBeatMaster"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // UI readouts (audio thread writes, UI thread reads)
    MasterEQ& getEQ() { return eq; }
    float getBandGainReductionDb (int band) const { return mbComp.getBandGainReductionDb (band); }
    float getLimiterGainReductionDb() const { return limiter.getGainReductionDb(); }
    float getMomentaryLUFS() const { return meter.getMomentaryLUFS(); }
    float getOutputPeakDb (int ch) const { return meter.getPeakDb (ch); }
    float getInputPeakDb (int ch) const;
    float getCorrelation() const { return meter.getCorrelation(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;

    // DSP chain
    juce::dsp::Gain<float> inputGain, outputGain;
    MasterEQ        eq;
    MultibandComp   mbComp;
    StereoImager    imager;
    Saturator       saturator;
    BrickwallLimiter limiter;
    LufsMeter       meter;

    // Internal stereo working buffer (uniform mono/stereo handling + chunking)
    juce::AudioBuffer<float> workBuf;
    int preparedMaxBlock = 512;
    double currentSampleRate = 44100.0;

    // Input auto-gain state
    float autoGainCorrDb = 0.0f;
    float inputRmsFollower = 0.0f;

    // Input peak metering
    std::atomic<float> inPeakDb[2];

    float getParam (const char* id) const;
    bool  getParamBool (const char* id) const;

    void processChunk (int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSBeatMasterProcessor)
};
