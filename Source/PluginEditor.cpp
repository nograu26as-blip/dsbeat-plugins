#include "PluginEditor.h"

//==============================================================================
DSBeatMasterEditor::DSBeatMasterEditor (DSBeatMasterProcessor& p)
    : AudioProcessorEditor (p), processor (p), meterBridge (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1200, 700);

    titleLabel.setText ("DS BEAT", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (30.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::chromeHi);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("MASTER SUITE", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (15.0f, juce::Font::bold));
    subtitleLabel.setColour (juce::Label::textColourId, DSBeatLookAndFeel::textDim);
    addAndMakeVisible (subtitleLabel);

    masterBypassButton.setButtonText ("MASTER BYPASS");
    addAndMakeVisible (masterBypassButton);
    buttonAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.getAPVTS(), IDs::masterBypass, masterBypassButton));

    addAndMakeVisible (tabs);
    tabs.addTab ("Input / EQ",   DSBeatLookAndFeel::panel, buildInputEqTab(),   false);
    tabs.addTab ("Multiband",    DSBeatLookAndFeel::panel, buildMultibandTab(), false);
    tabs.addTab ("Stereo / Sat", DSBeatLookAndFeel::panel, buildStereoSatTab(), false);
    tabs.addTab ("Limit / Out",  DSBeatLookAndFeel::panel, buildLimitOutTab(),  false);

    addAndMakeVisible (meterBridge);
}

DSBeatMasterEditor::~DSBeatMasterEditor()
{
    setLookAndFeel (nullptr);
}

void DSBeatMasterEditor::paint (juce::Graphics& g)
{
    g.fillAll (DSBeatLookAndFeel::bg);

    // Header chrome divider.
    juce::ColourGradient div (DSBeatLookAndFeel::chromeLo, 0.0f, 0.0f,
                              juce::Colour (0x002b2b32), (float) getWidth(), 0.0f, false);
    g.setGradientFill (div);
    g.fillRect (0, 55, getWidth(), 1);
}

void DSBeatMasterEditor::resized()
{
    titleLabel.setBounds (16, 8, 200, 36);
    subtitleLabel.setBounds (172, 14, 220, 28);
    masterBypassButton.setBounds (getWidth() - 190, 12, 174, 32);

    tabs.setBounds (8, 64, 872, 628);
    meterBridge.setBounds (888, 64, 304, 628);
}

//==============================================================================
juce::Slider* DSBeatMasterEditor::addKnob (juce::Component* parent, const juce::String& paramId,
                                           const juce::String& label, int x, int y, int cellW)
{
    auto* slider = new juce::Slider();
    slider->setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, cellW - 8, 18);
    slider->setBounds (x, y, cellW, 78);
    parent->addAndMakeVisible (slider);

    auto* text = new juce::Label();
    text->setText (label, juce::dontSendNotification);
    text->setFont (juce::Font (11.0f, juce::Font::bold));
    text->setColour (juce::Label::textColourId, DSBeatLookAndFeel::textDim);
    text->setJustificationType (juce::Justification::centred);
    text->setBounds (x, y + 78, cellW, 18);
    parent->addAndMakeVisible (text);

    sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.getAPVTS(), paramId, *slider));

    ownedComponents.emplace_back (slider);
    ownedComponents.emplace_back (text);
    return slider;
}

juce::ToggleButton* DSBeatMasterEditor::addToggle (juce::Component* parent, const juce::String& paramId,
                                                   const juce::String& label, int x, int y, int w)
{
    auto* button = new juce::ToggleButton (label);
    button->setBounds (x, y + 20, w, 36);
    parent->addAndMakeVisible (button);

    buttonAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.getAPVTS(), paramId, *button));

    ownedComponents.emplace_back (button);
    return button;
}

juce::Label* DSBeatMasterEditor::addSectionTitle (juce::Component* parent, const juce::String& text,
                                                  int x, int y, int w)
{
    auto* label = new juce::Label();
    label->setText (text, juce::dontSendNotification);
    label->setFont (juce::Font (13.0f, juce::Font::bold));
    label->setColour (juce::Label::textColourId, DSBeatLookAndFeel::chromeHi);
    label->setBounds (x, y, w, 20);
    parent->addAndMakeVisible (label);

    ownedComponents.emplace_back (label);
    return label;
}

//==============================================================================
juce::Component* DSBeatMasterEditor::buildInputEqTab()
{
    auto* tab = new juce::Component();

    auto* curve = new EQCurveDisplay();
    curve->setEQ (&processor.getEQ());
    curve->setBounds (10, 8, 852, 210);
    tab->addAndMakeVisible (curve);
    ownedComponents.emplace_back (curve);

    addSectionTitle (tab, "INPUT", 10, 226, 200);
    addKnob   (tab, IDs::inGain,   "Input Gain", 10, 250);
    addToggle (tab, IDs::autoGain, "AUTO GAIN", 112, 250, 130);

    addSectionTitle (tab, "EQUALIZER  —  4 BAND PARAMETRIC", 270, 226, 400);

    int x = 270;
    addKnob (tab, IDs::lowGain,  "Low Gain",  x, 250); x += 96;
    addKnob (tab, IDs::lowFreq,  "Low Freq",  x, 250); x += 96;
    addKnob (tab, IDs::mid1Gain, "Mid1 Gain", x, 250); x += 96;
    addKnob (tab, IDs::mid1Freq, "Mid1 Freq", x, 250); x += 96;
    addKnob (tab, IDs::mid1Q,    "Mid1 Q",    x, 250); x += 96;
    addKnob (tab, IDs::mid2Gain, "Mid2 Gain", x, 250);

    x = 270;
    constexpr int row2 = 360;
    addKnob (tab, IDs::mid2Freq, "Mid2 Freq", x, row2); x += 96;
    addKnob (tab, IDs::mid2Q,    "Mid2 Q",    x, row2); x += 96;
    addKnob (tab, IDs::highGain, "High Gain", x, row2); x += 96;
    addKnob (tab, IDs::highFreq, "High Freq", x, row2); x += 96;
    addToggle (tab, IDs::eqBypass, "EQ BYPASS", x, row2, 130);

    return tab;
}

juce::Component* DSBeatMasterEditor::buildMultibandTab()
{
    auto* tab = new juce::Component();

    addSectionTitle (tab, "CROSSOVER", 10, 8, 200);
    addKnob   (tab, IDs::xoverLow,  "Xover Low",  10, 32);
    addKnob   (tab, IDs::xoverHigh, "Xover High", 106, 32);
    addToggle (tab, IDs::mbBypass,  "MB BYPASS",  208, 32, 140);

    const char* bandTitles[3] = { "LOW BAND", "MID BAND", "HIGH BAND" };
    const int groupW = 276, groupH = 430, groupY = 140;

    for (int b = 0; b < 3; ++b)
    {
        const int gx = 10 + b * (groupW + 10);

        auto* group = new juce::GroupComponent (juce::String ("band") + juce::String (b),
                                                juce::String (bandTitles[b]));
        group->setBounds (gx, groupY, groupW, groupH);
        group->setColour (juce::GroupComponent::outlineColourId, DSBeatLookAndFeel::panelEdge);
        group->setColour (juce::GroupComponent::textColourId, DSBeatLookAndFeel::chromeHi);
        tab->addAndMakeVisible (group);
        ownedComponents.emplace_back (group);

        const int kx1 = gx + 14, kx2 = gx + 144;
        int ky = groupY + 34;
        addKnob (tab, IDs::bandParam ("thresh", b), "Threshold", kx1, ky);
        addKnob (tab, IDs::bandParam ("ratio",  b), "Ratio",     kx2, ky); ky += 112;
        addKnob (tab, IDs::bandParam ("attack", b), "Attack",    kx1, ky);
        addKnob (tab, IDs::bandParam ("release",b), "Release",   kx2, ky); ky += 112;
        addKnob (tab, IDs::bandParam ("makeup", b), "Makeup",    kx1, ky);
    }

    return tab;
}

juce::Component* DSBeatMasterEditor::buildStereoSatTab()
{
    auto* tab = new juce::Component();

    auto* imagerGroup = new juce::GroupComponent ("imager", "STEREO IMAGER  —  WIDTH PER BAND");
    imagerGroup->setBounds (10, 10, 852, 200);
    imagerGroup->setColour (juce::GroupComponent::outlineColourId, DSBeatLookAndFeel::panelEdge);
    imagerGroup->setColour (juce::GroupComponent::textColourId, DSBeatLookAndFeel::chromeHi);
    tab->addAndMakeVisible (imagerGroup);
    ownedComponents.emplace_back (imagerGroup);

    addKnob   (tab, IDs::widthLow,  "Width Low",  30, 60);
    addKnob   (tab, IDs::widthMid,  "Width Mid",  126, 60);
    addKnob   (tab, IDs::widthHigh, "Width High", 222, 60);
    addToggle (tab, IDs::imagerBypass, "IMAGER BYPASS", 330, 60, 170);

    auto* info = new juce::Label();
    info->setText ("0% = mono   ·   100% = original   ·   200% = extra wide",
                   juce::dontSendNotification);
    info->setFont (juce::Font (11.0f));
    info->setColour (juce::Label::textColourId, DSBeatLookAndFeel::textDim);
    info->setBounds (530, 84, 300, 20);
    tab->addAndMakeVisible (info);
    ownedComponents.emplace_back (info);

    auto* satGroup = new juce::GroupComponent ("sat", "HARMONIC SATURATOR  —  TUBE WARMTH");
    satGroup->setBounds (10, 230, 852, 200);
    satGroup->setColour (juce::GroupComponent::outlineColourId, DSBeatLookAndFeel::panelEdge);
    satGroup->setColour (juce::GroupComponent::textColourId, DSBeatLookAndFeel::chromeHi);
    tab->addAndMakeVisible (satGroup);
    ownedComponents.emplace_back (satGroup);

    addKnob   (tab, IDs::satDrive, "Drive", 30, 280);
    addKnob   (tab, IDs::satMix,   "Mix",   126, 280);
    addToggle (tab, IDs::satBypass, "SAT BYPASS", 234, 280, 150);

    return tab;
}

juce::Component* DSBeatMasterEditor::buildLimitOutTab()
{
    auto* tab = new juce::Component();

    auto* limGroup = new juce::GroupComponent ("lim", "BRICKWALL LIMITER");
    limGroup->setBounds (10, 10, 420, 220);
    limGroup->setColour (juce::GroupComponent::outlineColourId, DSBeatLookAndFeel::panelEdge);
    limGroup->setColour (juce::GroupComponent::textColourId, DSBeatLookAndFeel::chromeHi);
    tab->addAndMakeVisible (limGroup);
    ownedComponents.emplace_back (limGroup);

    addKnob   (tab, IDs::ceiling,    "Ceiling", 30, 60);
    addKnob   (tab, IDs::limRelease, "Release", 126, 60);
    addToggle (tab, IDs::limBypass,  "LIMITER BYPASS", 234, 60, 170);

    auto* outGroup = new juce::GroupComponent ("out", "OUTPUT");
    outGroup->setBounds (442, 10, 420, 220);
    outGroup->setColour (juce::GroupComponent::outlineColourId, DSBeatLookAndFeel::panelEdge);
    outGroup->setColour (juce::GroupComponent::textColourId, DSBeatLookAndFeel::chromeHi);
    tab->addAndMakeVisible (outGroup);
    ownedComponents.emplace_back (outGroup);

    addKnob (tab, IDs::outGain, "Output Gain", 462, 60);

    auto* tip = new juce::Label();
    tip->setText ("Tip: aim for -14 LUFS (streaming) to -9 LUFS (club) on the meter.\n"
                  "Keep true peaks under the ceiling — the limiter guarantees it.",
                  juce::dontSendNotification);
    tip->setFont (juce::Font (12.0f));
    tip->setColour (juce::Label::textColourId, DSBeatLookAndFeel::textDim);
    tip->setBounds (10, 250, 600, 44);
    tab->addAndMakeVisible (tip);
    ownedComponents.emplace_back (tip);

    return tab;
}
