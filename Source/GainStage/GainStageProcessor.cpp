#include "GainStageProcessor.h"
#include "GainStageEditor.h"

//==============================================================================
DSBeatGainStageProcessor::DSBeatGainStageProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    inPeakDb[0].store (-120.0f); inPeakDb[1].store (-120.0f);
    outPeakDb[0].store (-120.0f); outPeakDb[1].store (-120.0f);
}

DSBeatGainStageProcessor::~DSBeatGainStageProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DSBeatGainStageProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    juce::StringArray instrumentChoices;
    for (auto* n : GainStageIDs::instrumentNames)
        instrumentChoices.add (n);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { GainStageIDs::instrument, 1 }, "Instrument", instrumentChoices, 0));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { GainStageIDs::manualTarget, 1 }, "Manual Target",
        juce::NormalisableRange<float> (-30.0f, 0.0f, 0.1f), -6.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String (v, 1) + " dB"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));

    juce::StringArray lufsChoices { "Peak", "-18 LUFS", "-23 LUFS" };
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { GainStageIDs::lufsMode, 1 }, "Target Mode", lufsChoices, 0));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { GainStageIDs::autoMode, 1 }, "Auto Gain", true));

    auto trimParam = [] (const char* id, const juce::String& name)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float v, int) { return juce::String (v, 1) + " dB"; },
            [] (const juce::String& t) { return t.getFloatValue(); });
    };
    p.push_back (trimParam (GainStageIDs::trimL, "Trim L"));
    p.push_back (trimParam (GainStageIDs::trimR, "Trim R"));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { GainStageIDs::clipProtect, 1 }, "Clip Protect", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { GainStageIDs::masterBypass, 1 }, "Bypass", false));

    return { p.begin(), p.end() };
}

//==============================================================================
float DSBeatGainStageProcessor::getParam (const char* id) const
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();
    return 0.0f;
}

bool DSBeatGainStageProcessor::getParamBool (const char* id) const
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load() > 0.5f;
    return false;
}

int DSBeatGainStageProcessor::getInstrumentIndex() const
{
    return juce::jlimit (0, 11, (int) getParam (GainStageIDs::instrument));
}

float DSBeatGainStageProcessor::getEffectiveTargetPeakDb() const
{
    const int idx = getInstrumentIndex();
    if (idx == 0)
        return getParam (GainStageIDs::manualTarget);
    return GainStageIDs::instrumentTargetDb[idx];
}

float DSBeatGainStageProcessor::getEffectiveTargetLufs() const
{
    const int mode = (int) getParam (GainStageIDs::lufsMode);
    if (mode == 1) return -18.0f;
    if (mode == 2) return -23.0f;
    return -1.0f; // peak mode
}

float DSBeatGainStageProcessor::getInPeakDb (int ch) const
{
    return (ch >= 0 && ch < 2) ? inPeakDb[ch].load() : -120.0f;
}

float DSBeatGainStageProcessor::getOutPeakDb (int ch) const
{
    return (ch >= 0 && ch < 2) ? outPeakDb[ch].load() : -120.0f;
}

//==============================================================================
void DSBeatGainStageProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    preparedMaxBlock  = juce::jmax (samplesPerBlock, 64);
    workBuf.setSize (2, preparedMaxBlock);
    lufsMeter.prepare (sampleRate, 2, preparedMaxBlock);

    peakEnv = 0.0f;
    lufsSmooth = -70.0f;
    appliedGainDb.store (0.0f);
}

void DSBeatGainStageProcessor::releaseResources()
{
}

bool DSBeatGainStageProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::mono())
        return true;
    if (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo())
        return true;
    if (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo())
        return true;
    return false;
}

//==============================================================================
void DSBeatGainStageProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();

    if (numIn == 0 || n <= 0)
        return;

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    if (getParamBool (GainStageIDs::masterBypass))
    {
        if (numIn == 1 && numOut >= 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, n);
        return;
    }

    int offset = 0, remaining = n;
    while (remaining > 0)
    {
        const int chunk = juce::jmin (remaining, preparedMaxBlock);

        if (numIn >= 2)
        {
            workBuf.copyFrom (0, 0, buffer, 0, offset, chunk);
            workBuf.copyFrom (1, 0, buffer, 1, offset, chunk);
        }
        else
        {
            workBuf.copyFrom (0, 0, buffer, 0, offset, chunk);
            workBuf.copyFrom (1, 0, buffer, 0, offset, chunk);
        }

        processChunk (chunk);

        buffer.copyFrom (0, offset, workBuf, 0, 0, chunk);
        if (numOut >= 2)
            buffer.copyFrom (1, offset, workBuf, 1, 0, chunk);

        offset    += chunk;
        remaining -= chunk;
    }
}

void DSBeatGainStageProcessor::processChunk (int numSamples)
{
    //--- Analysis: block peak + loudness ---------------------------------------
    float blockPeak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = workBuf.getReadPointer (ch, 0);
        for (int i = 0; i < numSamples; ++i)
            blockPeak = juce::jmax (blockPeak, std::abs (d[i]));
    }

    const float blockDur = (float) numSamples / (float) currentSampleRate;

    // Slow peak follower: instant attack, ~2 s release.
    const float peakRelCoef = std::exp (-blockDur / 2.0f);
    peakEnv = juce::jmax (blockPeak, peakEnv * peakRelCoef);

    // Loudness follower on the K-weighted momentary reading.
    {
        juce::AudioBuffer<float> view (const_cast<float**> (workBuf.getArrayOfReadPointers()),
                                       2, 0, numSamples);
        lufsMeter.process (view);
    }
    const float lufsNow = lufsMeter.getMomentaryLUFS();
    const float lufsCoef = std::exp (-blockDur / 1.5f);
    lufsSmooth = lufsCoef * lufsSmooth + (1.0f - lufsCoef) * lufsNow;

    //--- Target ------------------------------------------------------------------
    const int lufsMode = (int) getParam (GainStageIDs::lufsMode);
    float neededDb = 0.0f;

    if (lufsMode == 0)
    {
        const float targetPeakDb = getEffectiveTargetPeakDb();
        const float measuredDb = juce::Decibels::gainToDecibels (juce::jmax (peakEnv, 1.0e-6f));
        neededDb = targetPeakDb - measuredDb;
    }
    else
    {
        const float targetLufs = (lufsMode == 1) ? -18.0f : -23.0f;
        neededDb = targetLufs - lufsSmooth;
    }
    neededDb = juce::jlimit (-30.0f, 30.0f, neededDb);

    //--- Smooth the applied gain ----------------------------------------------------
    float applied = appliedGainDb.load();
    if (getParamBool (GainStageIDs::autoMode))
    {
        const float sCoef = std::exp (-blockDur / 0.4f);
        applied = sCoef * applied + (1.0f - sCoef) * neededDb;
    }
    else
    {
        applied = 0.0f;
    }

    //--- Clip prevention ---------------------------------------------------------------
    float clipCorrDb = 0.0f;
    if (getParamBool (GainStageIDs::clipProtect))
    {
        const float trimMax = juce::jmax (getParam (GainStageIDs::trimL),
                                          getParam (GainStageIDs::trimR));
        const float totalLin = juce::Decibels::decibelsToGain (applied + trimMax);
        const float estPeak = peakEnv * totalLin;
        constexpr float safeLin = 0.97f; // ≈ -0.26 dB
        if (estPeak > safeLin && estPeak > 1.0e-6f)
            clipCorrDb = juce::Decibels::gainToDecibels (safeLin / estPeak); // negative
    }

    const float totalL = applied + clipCorrDb + getParam (GainStageIDs::trimL);
    const float totalR = applied + clipCorrDb + getParam (GainStageIDs::trimR);
    const float gL = juce::Decibels::decibelsToGain (totalL);
    const float gR = juce::Decibels::decibelsToGain (totalR);

    appliedGainDb.store (applied + clipCorrDb);

    //--- Apply ----------------------------------------------------------------------------
    float outPeakL = 0.0f, outPeakR = 0.0f;
    {
        auto* l = workBuf.getWritePointer (0, 0);
        auto* r = workBuf.getWritePointer (1, 0);
        for (int i = 0; i < numSamples; ++i)
        {
            l[i] *= gL; r[i] *= gR;
            outPeakL = juce::jmax (outPeakL, std::abs (l[i]));
            outPeakR = juce::jmax (outPeakR, std::abs (r[i]));
        }
    }

    //--- Metering (input peaks are pre-gain via peakEnv) ---------------------------
    const float inPeakDbNow = juce::Decibels::gainToDecibels (juce::jmax (peakEnv, 1.0e-6f), -120.0f);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float prev = inPeakDb[ch].load();
        inPeakDb[ch].store (inPeakDbNow > prev ? inPeakDbNow : juce::jmax (-120.0f, prev - 0.5f));

        const float outDb = juce::Decibels::gainToDecibels (
            ch == 0 ? outPeakL : outPeakR, -120.0f);
        const float oprev = outPeakDb[ch].load();
        outPeakDb[ch].store (outDb > oprev ? outDb : juce::jmax (-120.0f, oprev - 0.5f));
    }
}

//==============================================================================
juce::AudioProcessorEditor* DSBeatGainStageProcessor::createEditor()
{
    return new DSBeatGainStageEditor (*this);
}

void DSBeatGainStageProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DSBeatGainStageProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DSBeatGainStageProcessor();
}
