#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

// The parameters that make up a preset. (Loop / Monitor input are utility switches and are
// deliberately NOT part of a preset.)
struct PresetValues
{
    float room = 0.8f, damp = 0.5f, width = 1.0f, tail = 2.0f, gap = 0.0f,
          wet = 0.0f, dry = 0.0f, lowCut = 80.0f, highCut = 12000.0f;
    bool swellOnly = false;
};

struct Preset
{
    juce::String name;
    juce::String category;      // empty = top level (e.g. "Init")
    PresetValues values;
    bool isFactory = true;
    juce::File file;            // user presets only
};

/*  Factory presets are compiled in. User presets live as small XML files (*.revpreset) in
        Windows : %APPDATA%\Revora\Presets
        macOS   : ~/Library/Application Support/Revora/Presets
        Linux   : ~/.config/Revora/Presets
    so they can be backed up or shared by simply copying the files.

    Everything here is meant to be used from the message thread.                          */
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    static const std::vector<Preset>& getFactory();
    const std::vector<Preset>& getUser() const { return userPresets; }

    void rescanUser();
    juce::File getUserFolder() const;

    bool loadFactory (int index);
    bool loadUser (int index);
    void step (int direction);                       // -1 = previous, +1 = next (factory, then user)

    bool saveUser (const juce::String& name);        // saves the current settings, overwrites same name
    bool deleteUser (const juce::String& name);
    bool userExists (const juce::String& name) const;

    // what is currently selected (thread-safe, these are also read when the host saves state)
    juce::String getCurrentName() const;
    juce::String getCurrentCategory() const;
    bool isCurrentUser() const;
    bool isModified() const;                         // knobs moved since the preset was loaded

    // re-select a preset by name without touching the parameters (used when restoring a session)
    void restore (const juce::String& name, bool isUser);

private:
    PresetValues readCurrent() const;
    void apply (const PresetValues& v);
    void setCurrent (const Preset& p);

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> userPresets;

    mutable juce::CriticalSection lock;
    PresetValues reference;
    juce::String currentName, currentCategory;
    bool currentUser = false, hasCurrent = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
