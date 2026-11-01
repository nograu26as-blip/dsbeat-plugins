#include "StereoImager.h"

StereoImager::StereoImager() = default;

void StereoImager::prepare (const juce::dsp::ProcessSpec& spec, int maxBlockSize)
{
    xo.prepare (spec, maxBlockSize);

    const int nch = (int) spec.numChannels;
    lowBuf.setSize  (nch, maxBlockSize);
    midBuf.setSize  (nch, maxBlockSize);
    highBuf.setSize (nch, maxBlockSize);

    reset();
}

void StereoImager::reset()
{
    xo.reset();
}

void StereoImager::applyWidth (juce::AudioBuffer<float>& band, float width)
{
    if (band.getNumChannels() < 2)
        return;

    const int n = band.getNumSamples();
    auto* l = band.getWritePointer (0, 0);
    auto* r = band.getWritePointer (1, 0);

    for (int i = 0; i < n; ++i)
    {
        const float mid  = 0.5f * (l[i] + r[i]);
        const float side = 0.5f * (l[i] - r[i]) * width;
        l[i] = mid + side;
        r[i] = mid - side;
    }
}

void StereoImager::process (juce::AudioBuffer<float>& buffer, const Settings& s)
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || buffer.getNumChannels() < 2)
        return;

    xo.setFrequencies (s.xoverLowHz, s.xoverHighHz);
    xo.split (buffer, lowBuf, midBuf, highBuf);

    applyWidth (lowBuf,  s.widthLow);
    applyWidth (midBuf,  s.widthMid);
    applyWidth (highBuf, s.widthHigh);

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
