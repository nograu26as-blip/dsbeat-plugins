#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DSBeatMasterProcessor::DSBeatMasterProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    inPeakDb[0].store (-120.0f);
    inPeakDb[1].store (-120.0f);
}

DSBeatMasterProcessor::~DSBeatMasterProcessor() = default;

//==============================================================================
static juce::NormalisableRange<float> freqRange (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

static std::unique_ptr<juce::AudioParameterFloat> dbParam (const juce::String& id,
                                                           const juce::String& name,
                                                           float lo, float hi, float def)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name,
        juce::NormalisableRange<float> (lo, hi, 0.01f), def,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String (v, 1) + " dB"; },
        [] (const juce::String& t) { return t.getFloatValue(); });
}

static std::unique_ptr<juce::AudioParameterBool> boolParam (const juce::String& id,
                                                            const juce::String& name,
                                                            bool def)
{
    return std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { id, 1 }, name, def);
}

juce::AudioProcessorValueTreeState::ParameterLayout DSBeatMasterProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (boolParam (IDs::masterBypass, "Master Bypass", false));

    // Input
    p.push_back (dbParam (IDs::inGain, "Input Gain", -24.0f, 24.0f, 0.0f));
    p.push_back (boolParam (IDs::autoGain, "Auto Gain", false));

    // EQ
    p.push_back (boolParam (IDs::eqBypass, "EQ Bypass", false));
    p.push_back (dbParam (IDs::lowGain, "Low Gain", -15.0f, 15.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::lowFreq, 1 }, "Low Freq",
        freqRange (20.0f, 500.0f, 100.0f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " Hz"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));
    p.push_back (dbParam (IDs::mid1Gain, "Mid 1 Gain", -15.0f, 15.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::mid1Freq, 1 }, "Mid 1 Freq",
        freqRange (100.0f, 2000.0f, 400.0f), 400.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " Hz"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::mid1Q, 1 }, "Mid 1 Q",
        juce::NormalisableRange<float> (0.3f, 6.0f, 0.01f), 1.0f));
    p.push_back (dbParam (IDs::mid2Gain, "Mid 2 Gain", -15.0f, 15.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::mid2Freq, 1 }, "Mid 2 Freq",
        freqRange (800.0f, 12000.0f, 2500.0f), 2500.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " Hz"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::mid2Q, 1 }, "Mid 2 Q",
        juce::NormalisableRange<float> (0.3f, 6.0f, 0.01f), 1.0f));
    p.push_back (dbParam (IDs::highGain, "High Gain", -15.0f, 15.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::highFreq, 1 }, "High Freq",
        freqRange (4000.0f, 20000.0f, 10000.0f), 10000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " Hz"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));

    // Multiband
    p.push_back (boolParam (IDs::mbBypass, "Multiband Bypass", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::xoverLow, 1 }, "Crossover Low",
        freqRange (40.0f, 1000.0f, 200.0f), 200.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " Hz"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::xoverHigh, 1 }, "Crossover High",
        freqRange (1000.0f, 16000.0f, 3000.0f), 3000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " Hz"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));

    const char* bandNames[3] = { "Low", "Mid", "High" };
    for (int b = 0; b < 3; ++b)
    {
        juce::String bn = bandNames[b];
        p.push_back (dbParam (IDs::bandParam ("thresh", b), bn + " Thresh", -60.0f, 0.0f, -24.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { IDs::bandParam ("ratio", b), 1 }, bn + " Ratio",
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f), 2.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float v, int) { return juce::String (v, 1) + ":1"; },
            [] (const juce::String& t) { return t.getFloatValue(); }));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { IDs::bandParam ("attack", b), 1 }, bn + " Attack",
            freqRange (0.1f, 100.0f, 10.0f), 10.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float v, int) { return juce::String (v, 1) + " ms"; },
            [] (const juce::String& t) { return t.getFloatValue(); }));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { IDs::bandParam ("release", b), 1 }, bn + " Release",
            freqRange (10.0f, 1000.0f, 100.0f), 100.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float v, int) { return juce::String ((int) v) + " ms"; },
            [] (const juce::String& t) { return t.getFloatValue(); }));
        p.push_back (dbParam (IDs::bandParam ("makeup", b), bn + " Makeup", 0.0f, 24.0f, 0.0f));
    }

    // Stereo imager
    p.push_back (boolParam (IDs::imagerBypass, "Imager Bypass", false));
    auto widthParam = [] (const char* id, const juce::String& name)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f), 1.0f,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float v, int) { return juce::String ((int) (v * 100.0f)) + " %"; },
            [] (const juce::String& t) { return t.getFloatValue() / 100.0f; });
    };
    p.push_back (widthParam (IDs::widthLow,  "Width Low"));
    p.push_back (widthParam (IDs::widthMid,  "Width Mid"));
    p.push_back (widthParam (IDs::widthHigh, "Width High"));

    // Saturator
    p.push_back (boolParam (IDs::satBypass, "Saturator Bypass", false));
    auto pctParam = [] (const char* id, const juce::String& name, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), def,
            juce::String(), juce::AudioProcessorParameter::genericParameter,
            [] (float v, int) { return juce::String ((int) v) + " %"; },
            [] (const juce::String& t) { return t.getFloatValue(); });
    };
    p.push_back (pctParam (IDs::satDrive, "Drive", 0.0f));
    p.push_back (pctParam (IDs::satMix,   "Sat Mix", 100.0f));

    // Limiter
    p.push_back (boolParam (IDs::limBypass, "Limiter Bypass", false));
    p.push_back (dbParam (IDs::ceiling, "Ceiling", -12.0f, 0.0f, -1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { IDs::limRelease, 1 }, "Limiter Release",
        freqRange (10.0f, 500.0f, 50.0f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [] (float v, int) { return juce::String ((int) v) + " ms"; },
        [] (const juce::String& t) { return t.getFloatValue(); }));

    // Output
    p.push_back (dbParam (IDs::outGain, "Output Gain", -24.0f, 24.0f, 0.0f));

    return { p.begin(), p.end() };
}

//==============================================================================
float DSBeatMasterProcessor::getParam (const char* id) const
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();
    return 0.0f;
}

bool DSBeatMasterProcessor::getParamBool (const char* id) const
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load() > 0.5f;
    return false;
}

float DSBeatMasterProcessor::getInputPeakDb (int ch) const
{
    if (ch < 0 || ch > 1)
        return -120.0f;
    return inPeakDb[ch].load();
}

//==============================================================================
void DSBeatMasterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    preparedMaxBlock  = juce::jmax (samplesPerBlock, 64);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) preparedMaxBlock, 2 };

    inputGain.prepare (spec);
    outputGain.prepare (spec);
    inputGain.setRampDurationSeconds (0.05);
    outputGain.setRampDurationSeconds (0.05);

    eq.prepare (spec);
    mbComp.prepare (spec, preparedMaxBlock);
    imager.prepare (spec, preparedMaxBlock);
    limiter.prepare (sampleRate, 2, preparedMaxBlock);
    meter.prepare (sampleRate, 2, preparedMaxBlock);

    workBuf.setSize (2, preparedMaxBlock);

    autoGainCorrDb = 0.0f;
    inputRmsFollower = 0.0f;
    inPeakDb[0].store (-120.0f);
    inPeakDb[1].store (-120.0f);
}

void DSBeatMasterProcessor::releaseResources()
{
}

//==============================================================================
bool DSBeatMasterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
void DSBeatMasterProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();

    if (numIn == 0 || n <= 0)
        return;

    // Clear any extra output channels.
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    // Master bypass: pass through, still meter the signal.
    if (getParamBool (IDs::masterBypass))
    {
        if (numIn == 1 && numOut >= 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, n);

        meter.process (buffer);
        return;
    }

    // Process in chunks of preparedMaxBlock through the stereo work buffer.
    int offset = 0;
    int remaining = n;

    while (remaining > 0)
    {
        const int chunk = juce::jmin (remaining, preparedMaxBlock);

        // Fill work buffer: stereo in -> stereo, mono in -> dual mono.
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

        // Write back.
        buffer.copyFrom (0, offset, workBuf, 0, 0, chunk);
        if (numOut >= 2)
            buffer.copyFrom (1, offset, workBuf, 1, 0, chunk);

        offset    += chunk;
        remaining -= chunk;
    }
}

void DSBeatMasterProcessor::processChunk (int numSamples)
{
    //--- Input peak metering (pre gain) ---------------------------------------
    for (int ch = 0; ch < 2; ++ch)
    {
        float pk = 0.0f;
        auto* d = workBuf.getReadPointer (ch, 0);
        for (int i = 0; i < numSamples; ++i)
            pk = juce::jmax (pk, std::abs (d[i]));

        const float pkDb = juce::Decibels::gainToDecibels (pk, -120.0f);
        const float prev = inPeakDb[ch].load();
        // fast attack, slow release
        inPeakDb[ch].store (pkDb > prev ? pkDb : juce::jmax (-120.0f, prev - 0.35f));
    }

    //--- Input gain (+ optional auto-gain levelling to -18 dBFS RMS) ----------
    if (getParamBool (IDs::autoGain))
    {
        const float rms = (workBuf.getRMSLevel (0, 0, numSamples)
                         + workBuf.getRMSLevel (1, 0, numSamples)) * 0.5f;

        const float blockDur = (float) numSamples / (float) currentSampleRate;
        const float coef = std::exp (-blockDur / 1.0f); // ~1 s follower
        inputRmsFollower = coef * inputRmsFollower + (1.0f - coef) * rms;

        const float targetLin = juce::Decibels::decibelsToGain (-18.0f);
        float corrDb = juce::Decibels::gainToDecibels (
            targetLin / juce::jmax (inputRmsFollower, 1.0e-6f));
        corrDb = juce::jlimit (-24.0f, 24.0f, corrDb);

        // Smooth the correction itself to avoid pumping.
        const float sCoef = std::exp (-blockDur / 0.4f);
        autoGainCorrDb = sCoef * autoGainCorrDb + (1.0f - sCoef) * corrDb;
    }
    else
    {
        autoGainCorrDb = 0.0f;
    }

    inputGain.setGainDecibels (getParam (IDs::inGain) + autoGainCorrDb);

    juce::dsp::AudioBlock<float> block (workBuf);
    auto sub = block.getSubBlock (0, (size_t) numSamples);

    juce::dsp::ProcessContextReplacing<float> inCtx (sub);
    inputGain.process (inCtx);

    //--- EQ --------------------------------------------------------------------
    if (! getParamBool (IDs::eqBypass))
    {
        MasterEQ::Settings s;
        s.lowGainDb  = getParam (IDs::lowGain);
        s.lowFreqHz  = getParam (IDs::lowFreq);
        s.mid1GainDb = getParam (IDs::mid1Gain);
        s.mid1FreqHz = getParam (IDs::mid1Freq);
        s.mid1Q      = getParam (IDs::mid1Q);
        s.mid2GainDb = getParam (IDs::mid2Gain);
        s.mid2FreqHz = getParam (IDs::mid2Freq);
        s.mid2Q      = getParam (IDs::mid2Q);
        s.highGainDb = getParam (IDs::highGain);
        s.highFreqHz = getParam (IDs::highFreq);
        eq.updateCoefficients (s);

        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        eq.process (sub);
    }

    //--- Multiband compressor ---------------------------------------------------
    if (! getParamBool (IDs::mbBypass))
    {
        MultibandComp::Settings s;
        s.xoverLowHz  = getParam (IDs::xoverLow);
        s.xoverHighHz = getParam (IDs::xoverHigh);
        for (int b = 0; b < 3; ++b)
        {
            s.bands[b].threshDb  = getParam (IDs::bandParam ("thresh",  b).toRawUTF8());
            s.bands[b].ratio     = getParam (IDs::bandParam ("ratio",   b).toRawUTF8());
            s.bands[b].attackMs  = getParam (IDs::bandParam ("attack",  b).toRawUTF8());
            s.bands[b].releaseMs = getParam (IDs::bandParam ("release", b).toRawUTF8());
            s.bands[b].makeupDb  = getParam (IDs::bandParam ("makeup",  b).toRawUTF8());
        }

        // Operate on the first numSamples of workBuf.
        juce::AudioBuffer<float> view (workBuf.getArrayOfWritePointers(), 2, 0, numSamples);
        mbComp.process (view, s);
    }

    //--- Stereo imager -----------------------------------------------------------
    if (! getParamBool (IDs::imagerBypass))
    {
        StereoImager::Settings s;
        s.xoverLowHz  = getParam (IDs::xoverLow);
        s.xoverHighHz = getParam (IDs::xoverHigh);
        s.widthLow  = getParam (IDs::widthLow);
        s.widthMid  = getParam (IDs::widthMid);
        s.widthHigh = getParam (IDs::widthHigh);

        juce::AudioBuffer<float> view (workBuf.getArrayOfWritePointers(), 2, 0, numSamples);
        imager.process (view, s);
    }

    //--- Saturator ----------------------------------------------------------------
    if (! getParamBool (IDs::satBypass))
    {
        Saturator::Settings s;
        s.drive = getParam (IDs::satDrive) / 100.0f;
        s.mix   = getParam (IDs::satMix)   / 100.0f;

        juce::AudioBuffer<float> view (workBuf.getArrayOfWritePointers(), 2, 0, numSamples);
        saturator.process (view, s);
    }

    //--- Brickwall limiter ----------------------------------------------------------
    if (! getParamBool (IDs::limBypass))
    {
        BrickwallLimiter::Settings s;
        s.ceilingDb = getParam (IDs::ceiling);
        s.releaseMs = getParam (IDs::limRelease);

        juce::AudioBuffer<float> view (workBuf.getArrayOfWritePointers(), 2, 0, numSamples);
        limiter.process (view, s);
    }

    //--- Output gain -----------------------------------------------------------------
    outputGain.setGainDecibels (getParam (IDs::outGain));
    juce::dsp::ProcessContextReplacing<float> outCtx (sub);
    outputGain.process (outCtx);

    //--- Metering (post everything) ----------------------------------------------------
    {
        juce::AudioBuffer<float> view (const_cast<float**> (workBuf.getArrayOfReadPointers()), 2, 0, numSamples);
        meter.process (view);
    }
}

//==============================================================================
juce::AudioProcessorEditor* DSBeatMasterProcessor::createEditor()
{
    return new DSBeatMasterEditor (*this);
}

void DSBeatMasterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DSBeatMasterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DSBeatMasterProcessor();
}
