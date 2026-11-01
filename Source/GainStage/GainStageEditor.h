#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "GainStageProcessor.h"
#include "../UI/DSBeatLookAndFeel.h"

//==============================================================================
class DSBeatGainStageEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit DSBeatGainStageEditor (DSBeatGainStageProcessor&);
    ~DSBeatGainStageEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    DSBeatGainStageProcessor& processor;
    DSBeatLookAndFeel lookAndFeel;

    juce::Label titleLabel, subtitleLabel;

    // Big status display
    juce::Label instrumentNameLabel;
    juce::Label targetLabel;
    juce::Label gainLabel;

    // Instrument selector grid
    juce::TextButton instrumentButtons[12];

    // Target mode buttons
    juce::TextButton modeButtons[3];

    // Knobs + toggles
    juce::Slider manualTargetKnob, trimLKnob, trimRKnob;
    juce::Label manualTargetLabel, trimLLabel, trimRLabel;
    juce::ToggleButton autoButton, clipButton, bypassButton;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    // Before/after level meters
    class LevelMeters;
    std::unique_ptr<LevelMeters> meters;

    void setChoiceParam (const char* paramId, int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSBeatGainStageEditor)
};
