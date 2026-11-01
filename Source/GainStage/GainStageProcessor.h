#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../DSP/LufsMeter.h"

namespace GainStageIDs
{
    constexpr const char* instrument   = "instrument";   // choice: Manual + 11 instruments
    constexpr const char* manualTarget = "manualTarget"; // dB peak, -30..0
    constexpr const char* lufsMode     = "lufsMode";     // choice: Peak, -18 LUFS, -23 LUFS
    constexpr const char* autoMode     = "autoMode";     // bool
    constexpr const char* trimL        = "trimL";
    constexpr const char* trimR        = "trimR";
    constexpr const char* clipProtect  = "clipProtect";  // bool
    constexpr const char* masterBypass = "masterBypass"; // bool

    // PRÉ-MIX standard peak targets (dB), index = choice index (0 = Manual)
    constexpr const char* instrumentNames[12] =
        { "Manual", "Kick", "Baixo", "Snare", "Lead", "Vocal", "Synth",
          "Perc", "Hi-Hat", "Ride", "Pad", "FX" };

    constexpr float instrumentTargetDb[12] =
        { -6.0f, -10.0f, -12.0f, -14.0f, -14.0f, -14.0f, -16.0f,
          -18.0f, -18.0f, -18.0f, -20.0f, -22.0f };

    // Instrument accent colours for the UI level bars
    constexpr juce::uint32 instrumentColours[12] =
        { 0xff9a9aa2, 0xffe05252, 0xfffb923c, 0xfffacc15, 0xff4ade80, 0xff22d3ee,
          0xff60a5fa, 0xffa78bfa, 0xfff472b6, 0xff2dd4bf, 0xff818cf8, 0xffa3a3a3 };
}

//==============================================================================
class DSBeatGainStageProcessor : public juce::AudioProcessor
{
public:
    DSBeatGainStageProcessor();
    ~DSBeatGainStageProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DSBeatGainStage"; }
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

    // UI readouts
    float getInPeakDb (int ch) const;
    float getOutPeakDb (int ch) const;
    float getAppliedGainDb() const noexcept { return appliedGainDb.load(); }
    // Effective target: returns peak dB target, or lufsTarget via out-param when in LUFS mode.
    float getEffectiveTargetPeakDb() const;
    float getEffectiveTargetLufs() const; // -1 when in peak mode
    int   getInstrumentIndex() const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;

    juce::AudioBuffer<float> workBuf;
    int preparedMaxBlock = 512;
    double currentSampleRate = 44100.0;

    LufsMeter lufsMeter;

    // Analysis state
    float peakEnv = 0.0f;        // slow peak follower (linear)
    float lufsSmooth = -70.0f;   // slow loudness follower

    std::atomic<float> appliedGainDb { 0.0f };
    std::atomic<float> inPeakDb[2];
    std::atomic<float> outPeakDb[2];

    float getParam (const char* id) const;
    bool  getParamBool (const char* id) const;

    void processChunk (int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSBeatGainStageProcessor)
};
