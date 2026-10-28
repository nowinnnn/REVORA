#pragma once

#include "PluginProcessor.h"
#include "ModernLookAndFeel.h"

// ------------------------------------------------------------------------------------
//   [<] [>]  [ preset name                      TAG v ]  [Save] [Delete]        N factory | M user
//   - click the name for the full preset menu (factory categories, user presets, actions)
//   - Save turns the name field into an editor: Enter saves, Esc cancels
class PresetBar final : public juce::Component
{
public:
    explicit PresetBar (ReverseReverbProcessor& p) : proc (p)
    {
        addAndMakeVisible (prevButton);
        addAndMakeVisible (nextButton);
        addAndMakeVisible (nameBox);
        addAndMakeVisible (saveButton);
        addAndMakeVisible (deleteButton);
        addChildComponent (nameEditor);

        prevButton.onClick   = [this] { proc.presets.step (-1); refresh(); };
        nextButton.onClick   = [this] { proc.presets.step (+1); refresh(); };
        nameBox.onClick      = [this] { showMenu(); };
        saveButton.onClick   = [this] { beginSave(); };
        deleteButton.onClick = [this] { askDelete(); };

        nameEditor.setMultiLine (false);
        nameEditor.setReturnKeyStartsNewLine (false);
        nameEditor.setInputRestrictions (48);
        nameEditor.setJustification (juce::Justification::centredLeft);
        nameEditor.setIndents (16, 0);
        nameEditor.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        nameEditor.setTextToShowWhenEmpty ("Preset name", Theme::muted);
        nameEditor.setColour (juce::TextEditor::backgroundColourId, Theme::cardEdge);
        nameEditor.setColour (juce::TextEditor::textColourId, Theme::text);
        nameEditor.setColour (juce::TextEditor::highlightColourId, Theme::cyan.withAlpha (0.35f));
        nameEditor.setColour (juce::TextEditor::highlightedTextColourId, Theme::text);
        nameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, Theme::cyan);
        nameEditor.setColour (juce::CaretComponent::caretColourId, Theme::cyan);
        nameEditor.onReturnKey = [this] { commitSave(); };
        nameEditor.onEscapeKey = [this] { endSave(); };
        nameEditor.onFocusLost = [this] { endSave(); };

        refresh();
    }

    // called from the editor's timer
    void refresh()
    {
        auto& pm = proc.presets;
        const auto name = pm.getCurrentName();

        if (name.isEmpty())
            nameBox.set ("Custom", {}, false);
        else
            nameBox.set (name, pm.isCurrentUser() ? juce::String ("User") : pm.getCurrentCategory(), pm.isModified());

        deleteButton.setEnabled (pm.isCurrentUser());

        const int users = (int) pm.getUser().size();
        if (users != lastUserCount)
        {
            lastUserCount = users;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::muted);
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (juce::String ((int) PresetManager::getFactory().size()) + " factory  |  "
                        + juce::String ((int) proc.presets.getUser().size()) + " user",
                    getLocalBounds(), juce::Justification::centredRight);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        prevButton.setBounds (r.removeFromLeft (34));
        r.removeFromLeft (6);
        nextButton.setBounds (r.removeFromLeft (34));
        r.removeFromLeft (10);
        nameBox.setBounds (r.removeFromLeft (juce::jmin (r.getWidth(), 360)));
        nameEditor.setBounds (nameBox.getBounds());
        r.removeFromLeft (10);
        saveButton.setBounds (r.removeFromLeft (80));
        r.removeFromLeft (8);
        deleteButton.setBounds (r.removeFromLeft (80));
    }

private:
    // ---- the pill that shows the current preset ----
    class NameBox final : public juce::Component
    {
    public:
        NameBox() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

        void set (const juce::String& n, const juce::String& t, bool m)
        {
            if (n == name && t == tag && m == modified)
                return;
            name = n; tag = t; modified = m;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (isMouseOver() ? Theme::cardEdge.brighter (0.12f) : Theme::cardEdge);
            g.fillRoundedRectangle (b, b.getHeight() * 0.5f);

            auto inner = getLocalBounds().reduced (16, 0);

            // chevron
            auto chev = inner.removeFromRight (12).toFloat();
            juce::Path tri;
            tri.addTriangle (chev.getX(), chev.getCentreY() - 2.5f,
                             chev.getRight(), chev.getCentreY() - 2.5f,
                             chev.getCentreX(), chev.getCentreY() + 3.5f);
            g.setColour (Theme::muted);
            g.fillPath (tri);
            inner.removeFromRight (8);

            // category / "User" tag
            if (tag.isNotEmpty())
            {
                g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
                g.setColour (tag == "User" ? Theme::violet : Theme::muted);
                g.drawText (tag.toUpperCase(), inner.removeFromRight (110), juce::Justification::centredRight, true);
            }

            // "modified" dot
            if (modified)
            {
                g.setColour (Theme::amber);
                g.fillEllipse ((float) inner.getX(), (float) inner.getCentreY() - 3.5f, 7.0f, 7.0f);
            }
            inner.removeFromLeft (16);

            g.setColour (Theme::text);
            g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
            g.drawText (name, inner, juce::Justification::centredLeft, true);
        }

        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent& e) override  { if (e.mouseWasClicked() && onClick) onClick(); }

        std::function<void()> onClick;

    private:
        juce::String name, tag;
        bool modified = false;
    };

    // ---- menu ----
    enum MenuIds { kSave = 1, kDelete = 2, kFolder = 3, kFactoryBase = 1000, kUserBase = 5000 };

    void showMenu()
    {
        auto& pm = proc.presets;
        pm.rescanUser();

        const auto& fac = PresetManager::getFactory();
        const auto& usr = pm.getUser();
        const auto curName = pm.getCurrentName();
        const bool curUser = pm.isCurrentUser();

        juce::PopupMenu menu;

        // uncategorised factory presets (Init)
        for (int i = 0; i < (int) fac.size(); ++i)
            if (fac[(size_t) i].category.isEmpty())
                menu.addItem (kFactoryBase + i, fac[(size_t) i].name, true, ! curUser && curName == fac[(size_t) i].name);

        // one sub menu per category, in the order they are defined
        juce::StringArray cats;
        for (const auto& p : fac)
            if (p.category.isNotEmpty())
                cats.addIfNotAlreadyThere (p.category);

        for (const auto& cat : cats)
        {
            juce::PopupMenu sub;
            bool anyTicked = false;
            for (int i = 0; i < (int) fac.size(); ++i)
            {
                if (fac[(size_t) i].category != cat)
                    continue;
                const bool ticked = ! curUser && curName == fac[(size_t) i].name;
                anyTicked = anyTicked || ticked;
                sub.addItem (kFactoryBase + i, fac[(size_t) i].name, true, ticked);
            }
            menu.addSubMenu (cat, sub, true, nullptr, anyTicked);
        }

        menu.addSeparator();
        menu.addSectionHeader ("User presets");
        if (usr.empty())
            menu.addItem (-1, "(none saved yet)", false, false);
        for (int i = 0; i < (int) usr.size(); ++i)
            menu.addItem (kUserBase + i, usr[(size_t) i].name, true, curUser && curName.equalsIgnoreCase (usr[(size_t) i].name));

        menu.addSeparator();
        menu.addItem (kSave, "Save current settings as...");
        menu.addItem (kDelete, curUser ? "Delete \"" + curName + "\"" : juce::String ("Delete preset"), curUser);
        menu.addItem (kFolder, "Open user preset folder");

        menu.showMenuAsync (juce::PopupMenu::Options()
                                .withTargetComponent (&nameBox)
                                .withMinimumWidth (nameBox.getWidth()),
                            [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->handleMenu (result);
                            });
    }

    void handleMenu (int id)
    {
        auto& pm = proc.presets;

        if      (id == kSave)   beginSave();
        else if (id == kDelete) askDelete();
        else if (id == kFolder) { pm.getUserFolder().createDirectory(); pm.getUserFolder().startAsProcess(); }
        else if (id >= kUserBase)    pm.loadUser (id - kUserBase);
        else if (id >= kFactoryBase) pm.loadFactory (id - kFactoryBase);

        refresh();
    }

    // ---- saving ----
    void beginSave()
    {
        auto& pm = proc.presets;
        juce::String initial = pm.getCurrentName();
        if (initial.isEmpty())          initial = "My preset";
        else if (! pm.isCurrentUser())  initial += " copy";

        editing = true;
        nameBox.setVisible (false);
        nameEditor.setText (initial.substring (0, 48), false);
        nameEditor.setVisible (true);
        nameEditor.grabKeyboardFocus();
        nameEditor.selectAll();
    }

    void endSave()
    {
        if (! editing)
            return;

        editing = false;
        nameEditor.setVisible (false);
        nameBox.setVisible (true);
    }

    void commitSave()
    {
        const auto name = nameEditor.getText().trim();
        endSave();

        if (name.isEmpty())
            return;

        auto& pm = proc.presets;
        const bool sameAsCurrent = pm.isCurrentUser() && pm.getCurrentName().equalsIgnoreCase (name);

        if (pm.userExists (name) && ! sameAsCurrent)
        {
            juce::AlertWindow::showAsync (
                juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon,
                                                              "Replace preset?",
                                                              "A user preset named \"" + name + "\" already exists. Replace it?",
                                                              "Replace", "Cancel", this),
                [safe = juce::Component::SafePointer<PresetBar> (this), name] (int r)
                {
                    if (safe != nullptr && r == 1)
                        safe->writePreset (name);
                });
            return;
        }

        writePreset (name);
    }

    void writePreset (const juce::String& name)
    {
        if (! proc.presets.saveUser (name))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Cannot save preset",
                                                    "Could not write to:\n" + proc.presets.getUserFolder().getFullPathName());
        refresh();
    }

    void askDelete()
    {
        auto& pm = proc.presets;
        if (! pm.isCurrentUser())
            return;

        const auto name = pm.getCurrentName();
        juce::AlertWindow::showAsync (
            juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon,
                                                          "Delete preset?",
                                                          "Delete the user preset \"" + name + "\"? This cannot be undone.",
                                                          "Delete", "Cancel", this),
            [safe = juce::Component::SafePointer<PresetBar> (this), name] (int r)
            {
                if (safe != nullptr && r == 1)
                {
                    safe->proc.presets.deleteUser (name);
                    safe->refresh();
                }
            });
    }

    ReverseReverbProcessor& proc;

    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "Save" }, deleteButton { "Delete" };
    NameBox nameBox;
    juce::TextEditor nameEditor;

    bool editing = false;
    int lastUserCount = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
