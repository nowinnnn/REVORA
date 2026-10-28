#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

namespace Theme
{
    inline const juce::Colour bg       { 0xff0e1014 };
    inline const juce::Colour card     { 0xff161a21 };
    inline const juce::Colour cardEdge { 0xff252b36 };
    inline const juce::Colour text     { 0xffe8ecf2 };
    inline const juce::Colour muted    { 0xff7b8494 };
    inline const juce::Colour track    { 0xff2a303c };
    inline const juce::Colour cyan     { 0xff4de1ff };
    inline const juce::Colour violet   { 0xffa78bfa };
    inline const juce::Colour amber    { 0xffffb454 };
    inline const juce::Colour red      { 0xffff5d6c };
}

// Flat, dark, minimal: thin-arc knobs, pill buttons, switch-style toggles.
class ModernLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ModernLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, Theme::bg);

        setColour (juce::Slider::textBoxTextColourId, Theme::text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, Theme::cyan.withAlpha (0.35f));
        setColour (juce::Slider::rotarySliderFillColourId, Theme::cyan);
        setColour (juce::Slider::rotarySliderOutlineColourId, Theme::track);

        setColour (juce::Label::textColourId, Theme::text);

        setColour (juce::TextButton::buttonColourId, Theme::cardEdge);
        setColour (juce::TextButton::textColourOffId, Theme::text);
        setColour (juce::TextButton::textColourOnId, Theme::text);

        setColour (juce::ToggleButton::textColourId, Theme::text);
        setColour (juce::ToggleButton::tickColourId, Theme::cyan);

        setColour (juce::PopupMenu::backgroundColourId, Theme::card);
        setColour (juce::PopupMenu::textColourId, Theme::text);
        setColour (juce::PopupMenu::headerTextColourId, Theme::muted);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::cyan.withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, Theme::text);

        setColour (juce::TextEditor::backgroundColourId, Theme::cardEdge);
        setColour (juce::TextEditor::textColourId, Theme::text);
        setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId, Theme::cyan);
    }

    // pill-shaped text fields to match the buttons
    void fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& t) override
    {
        g.setColour (t.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, (float) h * 0.5f);
    }

    void drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& t) override
    {
        if (t.isEnabled() && t.hasKeyboardFocus (true))
        {
            g.setColour (t.findColour (juce::TextEditor::focusedOutlineColourId));
            g.drawRoundedRectangle (0.75f, 0.75f, (float) w - 1.5f, (float) h - 1.5f, ((float) h - 1.5f) * 0.5f, 1.5f);
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions (14.0f, juce::Font::bold));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override
    {
        auto b = button.getLocalBounds().toFloat().reduced (0.5f);
        auto c = backgroundColour;
        if (down)             c = c.darker (0.25f);
        else if (highlighted) c = c.brighter (0.15f);
        if (! button.isEnabled())
            c = c.withAlpha (0.4f);

        g.setColour (c);
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool) override
    {
        const auto r = button.getLocalBounds().toFloat();
        const float sw = 34.0f, sh = 18.0f;
        const bool on = button.getToggleState();

        juce::Rectangle<float> track (r.getX() + 2.0f, r.getCentreY() - sh * 0.5f, sw, sh);

        auto trackColour = on ? button.findColour (juce::ToggleButton::tickColourId) : Theme::track;
        if (highlighted)
            trackColour = trackColour.brighter (0.15f);

        g.setColour (trackColour);
        g.fillRoundedRectangle (track, sh * 0.5f);

        const float d = sh - 6.0f;
        const float kx = on ? track.getRight() - d - 3.0f : track.getX() + 3.0f;
        g.setColour (on ? Theme::bg : Theme::muted);
        g.fillEllipse (kx, track.getCentreY() - d * 0.5f, d, d);

        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (button.getButtonText(), r.withTrimmedLeft (sw + 12.0f), juce::Justification::centredLeft);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float thickness = 4.5f;
        const float arcR = radius - thickness * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const bool active = s.isMouseOverOrDragging();
        const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

        juce::Path trackPath;
        trackPath.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (trackPath, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.001f)
        {
            juce::Path valuePath;
            valuePath.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (active ? accent.brighter (0.2f) : accent);
            g.strokePath (valuePath, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float capR = radius * 0.58f;
        g.setColour (active ? Theme::cardEdge.brighter (0.2f) : Theme::cardEdge);
        g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

        const float sn = std::sin (angle), cs = std::cos (angle);
        juce::Line<float> tick (centre.x + capR * 0.35f * sn, centre.y - capR * 0.35f * cs,
                                centre.x + capR * 0.85f * sn, centre.y - capR * 0.85f * cs);
        g.setColour (Theme::text);
        g.drawLine (tick, 2.5f);
    }
};
