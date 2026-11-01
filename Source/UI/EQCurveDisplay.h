#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/MasterEQ.h"

// Live 4-band EQ magnitude curve on a log-frequency grid.
class EQCurveDisplay : public juce::Component, private juce::Timer
{
public:
    EQCurveDisplay();
    void setEQ (MasterEQ* eqToUse) { eq = eqToUse; }

    void paint (juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override { repaint(); }

    MasterEQ* eq = nullptr;
    static constexpr int numPoints = 160;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQCurveDisplay)
};
