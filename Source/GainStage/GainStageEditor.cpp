#include "GainStageEditor.h"

//==============================================================================
// Before/after level meters with per-instrument accent colour.
class DSBeatGainStageEditor::LevelMeters : public juce::Component
{
public:
    explicit LevelMeters (DSBeatGainStageProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (DSBeatLookAndFeel::panel);
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (DSBeatLookAndFeel::panelEdge);
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        const int idx = processor.getInstrumentIndex();
        const juce::Colour accent (GainStageIDs::instrumentColours[idx]);

        float y = bounds.getY() + 30.0f;
        const float x = bounds.getX() + 14.0f;
        const float w = bounds.getWidth() - 28.0f;
        constexpr float barH = 16.0f;

        drawMeter (g, { x, y, w, barH }, processor.getInPeakDb (0),  "IN  L",
                   juce::String (processor.getInPeakDb (0), 1) + " dB", accent);
        y += 44.0f;
        drawMeter (g, { x, y, w, barH }, processor.getInPeakDb (1),  "IN  R",
                   juce::String (processor.getInPeakDb (1), 1) + " dB", accent);
        y += 44.0f;
        drawMeter (g, { x, y, w, barH }, processor.getOutPeakDb (0), "OUT L",
                   juce::String (processor.getOutPeakDb (0), 1) + " dB", accent);
        y += 44.0f;
        drawMeter (g, { x, y, w, barH }, processor.getOutPeakDb (1), "OUT R",
                   juce::String (processor.getOutPeakDb (1), 1) + " dB", accent);
    }

private:
    void drawMeter (juce::Graphics& g, juce::Rectangle<float> area, float valueDb,
                    const juce::String& label, const juce::String& readout,
                    juce::Colour accent)
    {
        g.setColour (DSBeatLookAndFeel::textDim);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (label, area.getX(), area.getY() - 16, 60, 14,
                    juce::Justification::centredLeft, false);
        g.setColour (DSBeatLookAndFeel::text);
        g.drawText (readout, area.getRight() - 90, area.getY() - 16, 90, 14,
                    juce::Justification::centredRight, false);

        g.setColour (juce::Colour (0xff0d0d10));
        g.fillRoundedRectangle (area, 4.0f);

        const float t = juce::jlimit (0.0f, 1.0f, (valueDb + 60.0f) / 66.0f);
        const float fw = area.getWidth() * t;
        if (fw > 1.0f)
        {
            juce::Colour c = t > 0.97f ? DSBeatLookAndFeel::accentRed : accent;
            g.setColour (c);
            g.fillRoundedRectangle (area.getX(), area.getY(), fw, area.getHeight(), 4.0f);
        }

        // 0 dB marker
        const float zeroX = area.getX() + area.getWidth() * (60.0f / 66.0f);
        g.setColour (DSBeatLookAndFeel::panelEdge);
        g.drawVerticalLine ((int) zeroX, area.getY(), area.getBottom());

        g.setColour (DSBeatLookAndFeel::panelEdge);
        g.drawRoundedRectangle (area, 4.0f, 1.0f);
    }

    DSBeatGainStageProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeters)
};

//==============================================================================
DSBeatGainStageEditor::DSBeatGainStageEditor (DSBeatGainStageProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (900, 660);

    titleLabel.setText ("DS BEAT", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::chromeHi);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("GAIN STAGE  —  PRE-MIX", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    subtitleLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::textDim);
    addAndMakeVisible (subtitleLabel);

    //--- Big status display ------------------------------------------------------
    instrumentNameLabel.setFont (juce::Font (34.0f, juce::Font::bold));
    instrumentNameLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::chromeHi);
    instrumentNameLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (instrumentNameLabel);

    targetLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    targetLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::accentBlue);
    targetLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (targetLabel);

    gainLabel.setFont (juce::Font (18.0f, juce::Font::bold));
    gainLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::accentGreen);
    gainLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (gainLabel);

    //--- Instrument grid --------------------------------------------------------------
    for (int i = 0; i < 12; ++i)
    {
        auto* b = &instrumentButtons[i];
        juce::String target = (i == 0) ? juce::String ("manual")
            : juce::String (GainStageIDs::instrumentTargetDb[i], 0) + " dB";
        b->setButtonText (juce::String (GainStageIDs::instrumentNames[i]) + "\n" + target);
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff141417));
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2c2c34));
        b->setColour (juce::TextButton::textColourOffId, DSBeatLookAndFeel::textDim);
        b->setColour (juce::TextButton::textColourOnId, DSBeatLookAndFeel::chromeHi);
        b->onClick = [this, i] { setChoiceParam (GainStageIDs::instrument, i); };
        addAndMakeVisible (b);
    }

    //--- Target mode --------------------------------------------------------------------
    const char* modeNames[3] = { "PEAK", "-18 LUFS", "-23 LUFS" };
    for (int i = 0; i < 3; ++i)
    {
        auto* b = &modeButtons[i];
        b->setButtonText (modeNames[i]);
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff141417));
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2c2c34));
        b->setColour (juce::TextButton::textColourOffId, DSBeatLookAndFeel::textDim);
        b->setColour (juce::TextButton::textColourOnId, DSBeatLookAndFeel::chromeHi);
        b->onClick = [this, i] { setChoiceParam (GainStageIDs::lufsMode, i); };
        addAndMakeVisible (b);
    }

    //--- Knobs --------------------------------------------------------------------------------
    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& paramId,
                             const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 18);
        addAndMakeVisible (s);
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (11.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, DSBeatLookAndFeel::textDim);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
        sliderAttachments.push_back (
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.getAPVTS(), paramId, s));
    };

    setupKnob (manualTargetKnob, manualTargetLabel, GainStageIDs::manualTarget, "Manual Target");
    setupKnob (trimLKnob, trimLLabel, GainStageIDs::trimL, "Trim L");
    setupKnob (trimRKnob, trimRLabel, GainStageIDs::trimR, "Trim R");

    //--- Toggles ----------------------------------------------------------------------------------
    auto setupToggle = [this] (juce::ToggleButton& b, const juce::String& paramId,
                               const juce::String& text)
    {
        b.setButtonText (text);
        addAndMakeVisible (b);
        buttonAttachments.push_back (
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                processor.getAPVTS(), paramId, b));
    };

    setupToggle (autoButton,   GainStageIDs::autoMode,     "AUTO");
    setupToggle (clipButton,   GainStageIDs::clipProtect,  "CLIP PROTECT");
    setupToggle (bypassButton, GainStageIDs::masterBypass, "BYPASS");

    meters = std::make_unique<LevelMeters> (p);
    addAndMakeVisible (*meters);

    startTimerHz (30);
}

DSBeatGainStageEditor::~DSBeatGainStageEditor()
{
    setLookAndFeel (nullptr);
}

void DSBeatGainStageEditor::setChoiceParam (const char* paramId, int index)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (
            processor.getAPVTS().getParameter (paramId)))
    {
        p->setValueNotifyingHost (p->convertTo0to1 (index));
    }
}

void DSBeatGainStageEditor::timerCallback()
{
    const int idx = processor.getInstrumentIndex();
    for (int i = 0; i < 12; ++i)
        instrumentButtons[i].setToggleState (i == idx, juce::dontSendNotification);

    const int mode = juce::jlimit (0, 2, (int) processor.getAPVTS()
        .getRawParameterValue (GainStageIDs::lufsMode)->load());
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setToggleState (i == mode, juce::dontSendNotification);

    // Big display
    instrumentNameLabel.setText (GainStageIDs::instrumentNames[idx], juce::dontSendNotification);
    instrumentNameLabel.setColour (juce::Label::textColourId,
                                   juce::Colour (GainStageIDs::instrumentColours[idx]));

    const float targetLufs = processor.getEffectiveTargetLufs();
    if (targetLufs > -100.0f)
        targetLabel.setText ("TARGET " + juce::String (targetLufs, 0) + " LUFS",
                             juce::dontSendNotification);
    else
        targetLabel.setText ("TARGET " + juce::String (processor.getEffectiveTargetPeakDb(), 1) + " dB",
                             juce::dontSendNotification);

    const float g = processor.getAppliedGainDb();
    gainLabel.setText ((g >= 0 ? "+" : "") + juce::String (g, 1) + " dB AUTO",
                       juce::dontSendNotification);

    meters->repaint();
}

void DSBeatGainStageEditor::paint (juce::Graphics& g)
{
    g.fillAll (DSBeatLookAndFeel::bg);

    juce::ColourGradient div (DSBeatLookAndFeel::chromeLo, 0.0f, 0.0f,
                              juce::Colour (0x002b2b32), (float) getWidth(), 0.0f, false);
    g.setGradientFill (div);
    g.fillRect (0, 53, getWidth(), 1);

    // Display panel backdrop
    g.setColour (DSBeatLookAndFeel::panel);
    g.fillRoundedRectangle (10.0f, 62.0f, (float) getWidth() - 20.0f, 84.0f, 8.0f);
    g.setColour (DSBeatLookAndFeel::panelEdge);
    g.drawRoundedRectangle (10.0f, 62.0f, (float) getWidth() - 20.0f, 84.0f, 8.0f, 1.0f);
}

void DSBeatGainStageEditor::resized()
{
    titleLabel.setBounds (16, 6, 200, 36);
    subtitleLabel.setBounds (168, 12, 260, 28);

    instrumentNameLabel.setBounds (30, 70, 300, 68);
    targetLabel.setBounds (330, 70, 240, 68);
    gainLabel.setBounds (600, 70, 270, 68);

    // Instrument grid: 6 x 2
    const int gx = 10, gy = 158, gw = (getWidth() - 20 - 5 * 8) / 6, gh = 66;
    for (int i = 0; i < 12; ++i)
    {
        const int col = i % 6, row = i / 6;
        instrumentButtons[i].setBounds (gx + col * (gw + 8), gy + row * (gh + 8), gw, gh);
    }

    // Controls row
    int cy = 322;
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setBounds (10 + i * 118, cy, 110, 34);

    manualTargetKnob.setBounds (380, cy - 6, 96, 78);
    manualTargetLabel.setBounds (380, cy + 72, 96, 18);
    trimLKnob.setBounds (486, cy - 6, 96, 78);
    trimLLabel.setBounds (486, cy + 72, 96, 18);
    trimRKnob.setBounds (592, cy - 6, 96, 78);
    trimRLabel.setBounds (592, cy + 72, 96, 18);

    autoButton.setBounds (706, cy, 90, 36);
    clipButton.setBounds (706, cy + 42, 150, 36);
    bypassButton.setBounds (706, cy + 84, 110, 36);

    meters->setBounds (10, 452, getWidth() - 20, 198);
}
