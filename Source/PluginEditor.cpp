#include "PluginEditor.h"

ReverseReverbEditor::ReverseReverbEditor (ReverseReverbProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), waveform (p), dragOut (p), presetBar (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (presetBar);
    addAndMakeVisible (waveform);
    addAndMakeVisible (dragOut);
    addAndMakeVisible (captureButton);
    addAndMakeVisible (playButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loopButton);
    addAndMakeVisible (monitorButton);
    addAndMakeVisible (swellOnlyButton);

    // primary action = accent pill
    playButton.setColour (juce::TextButton::buttonColourId, Theme::cyan);
    playButton.setColour (juce::TextButton::textColourOffId, Theme::bg);
    playButton.setColour (juce::TextButton::textColourOnId, Theme::bg);

    swellOnlyButton.setColour (juce::ToggleButton::tickColourId, Theme::violet);

    loopAtt    = std::make_unique<BA> (proc.apvts, "loop",      loopButton);
    monitorAtt = std::make_unique<BA> (proc.apvts, "passthru",  monitorButton);
    swellAtt   = std::make_unique<BA> (proc.apvts, "swellOnly", swellOnlyButton);

    captureButton.onClick = [this]
    {
        if (proc.isCapturing()) proc.stopCapture();
        else                    proc.startCapture();
    };

    playButton.onClick = [this] { proc.togglePlay(); };

    saveButton.onClick = [this]
    {
        const auto rendered = proc.getRenderedFile();
        if (! rendered.existsAsFile())
            return;

        chooser = std::make_unique<juce::FileChooser> (
            "Save rendered audio",
            juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile (rendered.getFileName()),
            "*.wav");

        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                              [rendered] (const juce::FileChooser& fc)
                              {
                                  const auto target = fc.getResult();
                                  if (target != juce::File())
                                      rendered.copyFileTo (target.withFileExtension ("wav"));
                              });
    };

    // order matters: layoutKnobs() uses these indices
    addKnob ("room",    "Room",     Theme::cyan);    // 0  space
    addKnob ("damp",    "Damping",  Theme::cyan);    // 1
    addKnob ("width",   "Width",    Theme::cyan);    // 2
    addKnob ("tail",    "Length",   Theme::violet);  // 3  swell
    addKnob ("gap",     "Gap",      Theme::violet);  // 4
    addKnob ("lowcut",  "Low cut",  Theme::amber);   // 5  tone & mix
    addKnob ("highcut", "High cut", Theme::amber);   // 6
    addKnob ("wet",     "Wet",      Theme::amber);   // 7
    addKnob ("dry",     "Dry",      Theme::amber);   // 8

    setSize (900, 584);
    startTimerHz (30);
}

void ReverseReverbEditor::addKnob (const juce::String& id, const juce::String& text, juce::Colour colour)
{
    auto* k = knobs.add (new Knob());

    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 20);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, colour);

    k->label.setText (text.toUpperCase(), juce::dontSendNotification);
    k->label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    k->label.setColour (juce::Label::textColourId, Theme::muted);
    k->label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
}

void ReverseReverbEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    // ---- header ----
    {
        g.setColour (Theme::text);
        g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
        g.drawText ("REVORA", headerArea.getX(), headerArea.getY() + 8, 400, 32, juce::Justification::centredLeft);

        g.setColour (Theme::muted);
        g.setFont (juce::Font (juce::FontOptions (12.5f)));
        g.drawText ("Reverse reverb.  Drop in audio, shape the swell, drag it back into your DAW.",
                    headerArea.getX(), headerArea.getY() + 40, 640, 16, juce::Justification::centredLeft);

        juce::String text;
        juce::Colour col;
        if      (proc.isCapturing()) { text = "Capturing";         col = Theme::red;   }
        else if (proc.isRendering()) { text = "Rendering";         col = Theme::amber; }
        else if (proc.hasSource())   { text = "Ready";             col = Theme::cyan;  }
        else                         { text = "Waiting for audio"; col = Theme::muted; }

        auto pill = juce::Rectangle<int> (headerArea.getRight() - 160, headerArea.getCentreY() - 13, 160, 26).toFloat();
        g.setColour (col.withAlpha (0.14f));
        g.fillRoundedRectangle (pill, 13.0f);
        g.setColour (col);
        g.fillEllipse (pill.getX() + 12.0f, pill.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
        g.drawText (text, pill.withTrimmedLeft (28.0f), juce::Justification::centredLeft);
    }

    // ---- cards ----
    auto drawCard = [&g] (juce::Rectangle<int> r, const juce::String& title, juce::Colour accent)
    {
        auto f = r.toFloat();
        g.setColour (Theme::card);
        g.fillRoundedRectangle (f, 16.0f);
        g.setColour (Theme::cardEdge);
        g.drawRoundedRectangle (f.reduced (0.5f), 16.0f, 1.0f);

        g.setColour (accent);
        g.fillEllipse (f.getX() + 16.0f, f.getY() + 17.0f, 6.0f, 6.0f);
        g.setColour (Theme::muted);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (title, (int) f.getX() + 28, (int) f.getY() + 10, 200, 20, juce::Justification::centredLeft);
    };

    drawCard (cardSpace, "SPACE",      Theme::cyan);
    drawCard (cardSwell, "SWELL",      Theme::violet);
    drawCard (cardTone,  "TONE & MIX", Theme::amber);

    if (dragOver)
    {
        g.setColour (Theme::cyan);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 14.0f, 2.0f);
    }
}

void ReverseReverbEditor::layoutKnobs (juce::Rectangle<int> card, int first, int count, int reserveBottom)
{
    auto inner = card.reduced (14, 0);
    inner.removeFromTop (36);
    inner.removeFromBottom (12 + reserveBottom);

    const int cw = inner.getWidth() / count;
    for (int i = 0; i < count; ++i)
    {
        auto cell = inner.removeFromLeft (cw);
        auto* k = knobs[first + i];
        k->label.setBounds (cell.removeFromTop (18));
        k->slider.setBounds (cell);
    }
}

void ReverseReverbEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);

    headerArea = area.removeFromTop (64);
    presetBar.setBounds (area.removeFromTop (34));
    area.removeFromTop (10);
    waveform.setBounds (area.removeFromTop (180));
    area.removeFromTop (12);

    // transport row
    auto row = area.removeFromTop (40);
    captureButton.setBounds (row.removeFromLeft (150));
    row.removeFromLeft (8);
    playButton.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (16);
    loopButton.setBounds (row.removeFromLeft (80).withSizeKeepingCentre (80, 28));
    monitorButton.setBounds (row.removeFromLeft (140).withSizeKeepingCentre (140, 28));

    saveButton.setBounds (row.removeFromRight (110));
    row.removeFromRight (8);
    dragOut.setBounds (row.removeFromRight (160));

    area.removeFromTop (14);
    area.removeFromBottom (20);

    // cards
    const int u = (area.getWidth() - 24 - 3 * 28) / 9;
    cardSpace = area.removeFromLeft (3 * u + 28);
    area.removeFromLeft (12);
    cardSwell = area.removeFromLeft (2 * u + 28);
    area.removeFromLeft (12);
    cardTone = area;

    layoutKnobs (cardSpace, 0, 3, 0);
    layoutKnobs (cardSwell, 3, 2, 30);
    layoutKnobs (cardTone,  5, 4, 0);

    swellOnlyButton.setBounds (cardSwell.getX() + 16, cardSwell.getBottom() - 38, cardSwell.getWidth() - 32, 28);
}

bool ReverseReverbEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    static const juce::StringArray exts { ".wav", ".aif", ".aiff", ".flac", ".ogg", ".mp3" };

    for (auto& f : files)
        if (exts.contains (juce::File (f).getFileExtension().toLowerCase()))
            return true;

    return false;
}

void ReverseReverbEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();

    for (auto& f : files)
        if (proc.loadFile (juce::File (f)))
            return;

    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                            "Cannot open file",
                                            "Unsupported format, or the file is longer than 60 seconds.");
}

void ReverseReverbEditor::timerCallback()
{
    const bool capturing = proc.isCapturing();
    if (capturing != lastCapturing)
    {
        lastCapturing = capturing;
        captureButton.setButtonText (capturing ? "Stop capture" : "Capture from DAW");
        captureButton.setColour (juce::TextButton::buttonColourId, capturing ? Theme::red : Theme::cardEdge);
        captureButton.setColour (juce::TextButton::textColourOffId, capturing ? Theme::bg : Theme::text);
        captureButton.repaint();
    }

    const bool playing = proc.isPlaying();
    if (playing != lastPlaying)
    {
        lastPlaying = playing;
        playButton.setButtonText (playing ? "Stop" : "Play");
    }

    const int rv = proc.getRenderVersion();
    if (rv != lastRenderVersion)
    {
        lastRenderVersion = rv;
        dragOut.repaint();
    }

    presetBar.refresh();
    waveform.refresh();
    repaint (headerArea);
}
