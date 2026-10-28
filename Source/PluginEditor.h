#pragma once

#include "PluginProcessor.h"
#include "ModernLookAndFeel.h"
#include "PresetBar.h"

// ------------------------------------------------------------------------------------
// Rendered audio, position of the dry sound and the playhead.
class WaveformView final : public juce::Component
{
public:
    explicit WaveformView (ReverseReverbProcessor& p) : proc (p) {}

    void refresh()
    {
        const int ver = proc.getRenderVersion();
        if (ver != cachedVersion)
        {
            cachedVersion = ver;
            rendered = proc.getRendered();
            dryOffset = proc.getDryOffsetSamples();
            rebuildPeaks();
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (Theme::card);
        g.fillRoundedRectangle (b, 16.0f);
        g.setColour (Theme::cardEdge);
        g.drawRoundedRectangle (b.reduced (0.5f), 16.0f, 1.0f);

        g.setFont (juce::Font (juce::FontOptions (14.0f)));

        if (proc.isCapturing())
        {
            g.setColour (Theme::red);
            g.drawText ("Capturing audio from the DAW...  start playback, then press Stop capture.",
                        getLocalBounds(), juce::Justification::centred);
            return;
        }

        if (rendered == nullptr || peaks.empty())
        {
            g.setColour (Theme::muted);
            g.drawText (proc.hasSource() ? "Rendering..."
                                         : "Drop an audio file here, or use  Capture from DAW",
                        getLocalBounds(), juce::Justification::centred);
            return;
        }

        const auto inner = b.reduced (16.0f, 18.0f);
        const float mid  = inner.getCentreY();
        const float half = inner.getHeight() * 0.5f;
        const int len = juce::jmax (1, rendered->getNumSamples());
        const float dx = inner.getX() + inner.getWidth() * (float) dryOffset / (float) len;

        // swell region tint
        g.setColour (Theme::cyan.withAlpha (0.06f));
        g.fillRoundedRectangle (juce::Rectangle<float> (inner.getX(), b.getY() + 8.0f,
                                                        juce::jmax (0.0f, dx - inner.getX()), b.getHeight() - 16.0f), 8.0f);

        g.setColour (Theme::track);
        g.fillRect (inner.getX(), mid - 0.5f, inner.getWidth(), 1.0f);

        // waveform
        juce::Path path;
        const float step = inner.getWidth() / (float) (peaks.size() - 1);
        path.startNewSubPath (inner.getX(), mid - peaks[0].second * half * displayScale);
        for (size_t i = 1; i < peaks.size(); ++i)
            path.lineTo (inner.getX() + (float) i * step, mid - peaks[i].second * half * displayScale);
        for (size_t i = peaks.size(); i-- > 0;)
            path.lineTo (inner.getX() + (float) i * step, mid - peaks[i].first * half * displayScale);
        path.closeSubPath();

        g.setGradientFill (juce::ColourGradient (Theme::cyan, inner.getX(), 0.0f,
                                                 Theme::violet, inner.getRight(), 0.0f, false));
        g.fillPath (path);

        // labels
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        if (dx - inner.getX() > 60.0f)
        {
            g.setColour (Theme::cyan.withAlpha (0.8f));
            g.drawText ("SWELL", (int) inner.getX() + 8, (int) b.getY() + 12, 60, 16, juce::Justification::left);
        }

        // dry marker
        g.setColour (Theme::amber);
        g.fillRect (dx - 1.0f, inner.getY() - 6.0f, 2.0f, inner.getHeight() + 12.0f);
        juce::Rectangle<float> tag (dx + 6.0f, b.getY() + 10.0f, 38.0f, 18.0f);
        g.fillRoundedRectangle (tag, 9.0f);
        g.setColour (Theme::bg);
        g.drawText ("DRY", tag, juce::Justification::centred);

        // playhead
        if (proc.isPlaying())
        {
            const float px = inner.getX() + inner.getWidth() * proc.getPlayheadFraction();
            g.setColour (juce::Colours::white);
            g.fillRect (px - 0.75f, b.getY() + 6.0f, 1.5f, b.getHeight() - 12.0f);
        }

        g.setColour (Theme::muted);
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (proc.getSourceName() + (proc.isRendering() ? "   rendering..." : ""),
                    getLocalBounds().reduced (16, 6), juce::Justification::bottomLeft);
    }

private:
    void rebuildPeaks()
    {
        peaks.clear();
        displayScale = 1.0f;
        if (rendered == nullptr || rendered->getNumSamples() == 0)
            return;

        const int bins = 1024;
        const int len = rendered->getNumSamples();
        peaks.resize ((size_t) bins);
        float absMax = 0.0f;

        for (int i = 0; i < bins; ++i)
        {
            const int a = (int) ((juce::int64) i * len / bins);
            const int e = juce::jmax (a + 1, (int) ((juce::int64) (i + 1) * len / bins));
            float mn = 0.0f, mx = 0.0f;
            for (int ch = 0; ch < rendered->getNumChannels(); ++ch)
            {
                const float* d = rendered->getReadPointer (ch);
                for (int k = a; k < e && k < len; ++k)
                {
                    mn = juce::jmin (mn, d[k]);
                    mx = juce::jmax (mx, d[k]);
                }
            }
            peaks[(size_t) i] = { mn, mx };
            absMax = juce::jmax (absMax, -mn, mx);
        }

        if (absMax > 1.0e-4f)
            displayScale = 0.95f / absMax;   // always show the wave at a readable size
    }

    ReverseReverbProcessor& proc;
    std::shared_ptr<const juce::AudioBuffer<float>> rendered;
    std::vector<std::pair<float, float>> peaks;
    float displayScale = 1.0f;
    int cachedVersion = -1;
    int dryOffset = 0;
};

// ------------------------------------------------------------------------------------
// Drag this onto a DAW track / sample slot to drop the rendered WAV file there.
class DragOutBox final : public juce::Component
{
public:
    explicit DragOutBox (ReverseReverbProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const bool ready = proc.getRenderedFile().existsAsFile();
        auto b = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (isMouseOver() && ready ? Theme::cyan.withAlpha (0.22f) : Theme::cardEdge.withAlpha (ready ? 1.0f : 0.5f));
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
        g.setColour (ready ? Theme::cyan.withAlpha (0.7f) : Theme::track);
        g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.0f);

        g.setColour (ready ? Theme::text : Theme::muted);
        g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        g.drawText ("Drag WAV to DAW", getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { repaint(); }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 6)
            return;

        const auto f = proc.getRenderedFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

private:
    ReverseReverbProcessor& proc;
};

// ------------------------------------------------------------------------------------
class ReverseReverbEditor final : public juce::AudioProcessorEditor,
                                  public juce::FileDragAndDropTarget,
                                  private juce::Timer
{
public:
    explicit ReverseReverbEditor (ReverseReverbProcessor&);
    ~ReverseReverbEditor() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true;  repaint(); }
    void fileDragExit  (const juce::StringArray&) override           { dragOver = false; repaint(); }

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (const juce::String& id, const juce::String& text, juce::Colour colour);
    void layoutKnobs (juce::Rectangle<int> card, int first, int count, int reserveBottom);
    void timerCallback() override;

    ModernLookAndFeel lnf;   // must outlive the child components
    ReverseReverbProcessor& proc;

    WaveformView waveform;
    DragOutBox dragOut;
    PresetBar presetBar;

    juce::TextButton captureButton { "Capture from DAW" };
    juce::TextButton playButton { "Play" };
    juce::TextButton saveButton { "Save WAV" };
    juce::ToggleButton loopButton { "Loop" };
    juce::ToggleButton monitorButton { "Monitor input" };
    juce::ToggleButton swellOnlyButton { "Swell only" };

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BA> loopAtt, monitorAtt, swellAtt;

    juce::OwnedArray<Knob> knobs;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> headerArea, cardSpace, cardSwell, cardTone;
    bool dragOver = false;
    bool lastCapturing = false, lastPlaying = false;
    int lastRenderVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseReverbEditor)
};
