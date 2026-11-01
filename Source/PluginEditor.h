#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/DSBeatLookAndFeel.h"
#include "UI/EQCurveDisplay.h"
#include "UI/MeterBridge.h"

//==============================================================================
class DSBeatMasterEditor : public juce::AudioProcessorEditor
{
public:
    explicit DSBeatMasterEditor (DSBeatMasterProcessor&);
    ~DSBeatMasterEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    DSBeatMasterProcessor& processor;
    DSBeatLookAndFeel lookAndFeel;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    MeterBridge meterBridge;

    juce::Label titleLabel, subtitleLabel;
    juce::ToggleButton masterBypassButton;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::Component>> ownedComponents;

    juce::Slider* addKnob (juce::Component* parent, const juce::String& paramId,
                           const juce::String& label, int x, int y, int cellW = 96);
    juce::ToggleButton* addToggle (juce::Component* parent, const juce::String& paramId,
                                   const juce::String& label, int x, int y, int w = 130);
    juce::Label* addSectionTitle (juce::Component* parent, const juce::String& text,
                                  int x, int y, int w);

    juce::Component* buildInputEqTab();
    juce::Component* buildMultibandTab();
    juce::Component* buildStereoSatTab();
    juce::Component* buildLimitOutTab();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSBeatMasterEditor)
};
