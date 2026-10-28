#include "PresetManager.h"
#include <array>

namespace
{
    const char* const kIds[10] = { "room", "damp", "width", "tail", "gap",
                                   "wet", "dry", "lowcut", "highcut", "swellOnly" };

    std::array<float, 10> toArray (const PresetValues& v)
    {
        return { v.room, v.damp, v.width, v.tail, v.gap, v.wet, v.dry, v.lowCut, v.highCut,
                 v.swellOnly ? 1.0f : 0.0f };
    }

    // ---------------------------------------------------------------------------------
    //                                 FACTORY PRESETS
    //  room, damp, width : reverb character         tail : swell length (s)   gap : ms
    //  wet / dry : dB    lowCut / highCut : Hz      swell : keep only the part BEFORE the dry sound
    //  A dry level of -60 dB mutes the dry signal, which gives you the pure swell to layer
    //  under the untouched original in your DAW.
    // ---------------------------------------------------------------------------------
    std::vector<Preset> buildFactory()
    {
        std::vector<Preset> list;

        auto add = [&list] (const char* cat, const char* name,
                            float room, float damp, float width, float tail, float gap,
                            float wet, float dry, float lowCut, float highCut, bool swell)
        {
            Preset p;
            p.category = cat;
            p.name = name;
            p.values = { room, damp, width, tail, gap, wet, dry, lowCut, highCut, swell };
            p.isFactory = true;
            list.push_back (std::move (p));
        };

        //   category         name                       room  damp  width tail  gap   wet   dry   lc     hc      swell
        add ("",              "Init",                    0.80f, 0.50f, 1.0f, 2.00f,   0.f,  0.f,   0.f,   80.f, 12000.f, false);

        // ---- Vocals ----
        add ("Vocals",        "Vocal Swell Classic",     0.75f, 0.55f, 1.0f, 1.80f,   0.f,  0.f,   0.f,  120.f, 11000.f, false);
        add ("Vocals",        "Whisper Rise",            0.60f, 0.60f, 0.9f, 1.20f,   0.f, -3.f,   0.f,  150.f, 10000.f, false);
        add ("Vocals",        "Dreamy Vocal Bloom",      0.90f, 0.45f, 1.0f, 3.50f,  40.f, -2.f,  -2.f,  140.f,  9500.f, false);
        add ("Vocals",        "Vocal Pre-Verb Gap",      0.70f, 0.50f, 1.0f, 1.50f, 120.f,  0.f,   0.f,  120.f, 12000.f, true);
        add ("Vocals",        "Ghost Swell (Wet Only)",  0.85f, 0.60f, 1.0f, 2.50f,   0.f,  0.f, -60.f,  200.f,  8000.f, true);
        add ("Vocals",        "Intimate Breath",         0.45f, 0.70f, 0.7f, 0.80f,   0.f, -2.f,   0.f,  180.f,  9000.f, false);
        add ("Vocals",        "Choir Cloud",             0.95f, 0.35f, 1.0f, 5.00f,  60.f, -3.f,   0.f,  100.f, 12000.f, false);
        add ("Vocals",        "Telephone Rise",          0.50f, 0.70f, 0.6f, 1.00f,   0.f,  0.f,   0.f,  600.f,  4500.f, false);

        // ---- Drums & Percussion ----
        add ("Drums",         "Snare Reverse Crash",     0.85f, 0.40f, 1.0f, 1.60f,   0.f,  0.f,   0.f,  200.f, 14000.f, true);
        add ("Drums",         "Snare Short Suck",        0.50f, 0.50f, 0.9f, 0.50f,   0.f,  0.f,   0.f,  250.f, 13000.f, true);
        add ("Drums",         "Kick Suck-In",            0.40f, 0.60f, 0.6f, 0.35f,   0.f, -2.f,   0.f,   30.f,  6000.f, true);
        add ("Drums",         "Tom Whoosh",              0.70f, 0.50f, 1.0f, 1.20f,   0.f,  0.f,   0.f,  100.f, 10000.f, true);
        add ("Drums",         "Hi-Hat Shimmer Rise",     0.60f, 0.20f, 1.0f, 1.00f,   0.f, -2.f,   0.f,  800.f, 18000.f, true);
        add ("Drums",         "Clap Reverse Tail",       0.65f, 0.40f, 1.0f, 0.90f,  20.f,  0.f,   0.f,  300.f, 14000.f, true);
        add ("Drums",         "Perc Pre-Verb Gate",      0.55f, 0.50f, 0.8f, 0.60f,  80.f,  0.f,   0.f,  200.f, 12000.f, false);
        add ("Drums",         "Cymbal Backwash",         0.95f, 0.30f, 1.0f, 4.00f,   0.f,  0.f,   0.f,  250.f, 16000.f, true);

        // ---- Guitars & Keys ----
        add ("Guitars & Keys", "Backwards Guitar Bloom", 0.80f, 0.50f, 1.0f, 2.50f,   0.f,  0.f,   0.f,  100.f,  9000.f, false);
        add ("Guitars & Keys", "Clean Guitar Wash",      0.85f, 0.40f, 1.0f, 3.00f,  30.f, -1.f,   0.f,  120.f, 10000.f, false);
        add ("Guitars & Keys", "Ambient Guitar Swell",   0.90f, 0.50f, 1.0f, 4.50f,   0.f, -2.f,  -1.f,  140.f,  8500.f, false);
        add ("Guitars & Keys", "Piano Reverse Halo",     0.85f, 0.45f, 1.0f, 3.00f,   0.f, -2.f,   0.f,   90.f, 12000.f, false);
        add ("Guitars & Keys", "Felt Piano Sigh",        0.70f, 0.60f, 0.9f, 2.00f,  60.f, -3.f,   0.f,  100.f,  7500.f, false);
        add ("Guitars & Keys", "Electric Piano Glass",   0.75f, 0.30f, 1.0f, 2.20f,   0.f, -2.f,   0.f,  200.f, 14000.f, false);
        add ("Guitars & Keys", "Bass Subtle Rise",       0.50f, 0.70f, 0.5f, 1.00f,   0.f, -4.f,   0.f,   25.f,  3500.f, true);

        // ---- Synths & FX ----
        add ("Synths & FX",   "Pad Expansion",           0.95f, 0.30f, 1.0f, 6.00f,   0.f, -2.f,   0.f,  100.f, 14000.f, false);
        add ("Synths & FX",   "Pluck Reverse Echo",      0.70f, 0.35f, 1.0f, 1.50f,   0.f,  0.f,   0.f,  150.f, 15000.f, true);
        add ("Synths & FX",   "Arp Smear",               0.80f, 0.30f, 1.0f, 2.50f, 100.f, -3.f,   0.f,  200.f, 16000.f, false);
        add ("Synths & FX",   "Riser Maker (Wet Only)",  1.00f, 0.25f, 1.0f, 8.00f,   0.f,  0.f, -60.f,  150.f, 15000.f, true);
        add ("Synths & FX",   "Gap Pump",                0.70f, 0.50f, 1.0f, 1.00f, 250.f,  0.f,   0.f,  120.f, 12000.f, false);
        add ("Synths & FX",   "Lo-Fi Tape Rewind",       0.60f, 0.85f, 0.8f, 1.40f,   0.f, -1.f,   0.f,  250.f,  3800.f, false);
        add ("Synths & FX",   "Glitch Short Reverse",    0.30f, 0.50f, 0.7f, 0.25f,   0.f,  0.f,   0.f,  150.f, 12000.f, true);

        // ---- Cinematic & Ambient ----
        add ("Cinematic",     "Cinematic Impact Rise",   1.00f, 0.30f, 1.0f, 7.00f,   0.f,  0.f,   0.f,   80.f, 13000.f, true);
        add ("Cinematic",     "Horror Reverse",          0.85f, 0.75f, 1.0f, 4.00f,   0.f,  0.f,   0.f,   60.f,  3500.f, false);
        add ("Cinematic",     "Dark Hall Swell",         0.95f, 0.70f, 1.0f, 5.00f,   0.f, -2.f,   0.f,   50.f,  5000.f, false);
        add ("Cinematic",     "Heavenly Wash",           1.00f, 0.20f, 1.0f, 6.00f,   0.f, -2.f,  -3.f,  200.f, 16000.f, false);
        add ("Cinematic",     "Ethereal Drift",          0.90f, 0.40f, 1.0f, 4.50f,  80.f, -2.f,  -2.f,  180.f, 12000.f, false);
        add ("Cinematic",     "Underwater",              0.80f, 0.90f, 1.0f, 3.00f,   0.f, -1.f,   0.f,   40.f,  2000.f, false);
        add ("Cinematic",     "Trailer Whoosh",          1.00f, 0.45f, 1.0f, 3.00f,   0.f,  0.f,   0.f,  150.f, 15000.f, true);

        // ---- Subtle & Utility ----
        add ("Subtle",        "Subtle Pre-Echo",         0.35f, 0.50f, 0.8f, 0.40f,   0.f, -6.f,   0.f,  150.f, 12000.f, false);
        add ("Subtle",        "Small Room Reverse",      0.30f, 0.60f, 0.7f, 0.50f,   0.f, -3.f,   0.f,  120.f, 10000.f, false);
        add ("Subtle",        "Tight Drum Room",         0.40f, 0.50f, 0.8f, 0.35f,   0.f, -4.f,   0.f,  150.f, 11000.f, false);
        add ("Subtle",        "Wide Stereo Halo",        0.70f, 0.40f, 1.0f, 1.80f,   0.f, -4.f,   0.f,  250.f, 16000.f, false);
        add ("Subtle",        "Mono Compatible Swell",   0.75f, 0.50f, 0.0f, 2.00f,   0.f, -2.f,   0.f,  100.f, 11000.f, false);
        add ("Subtle",        "Wet Only (Full Reverb)",  0.80f, 0.50f, 1.0f, 2.50f,   0.f,  0.f, -60.f,  100.f, 12000.f, false);

        return list;
    }

    bool readUserFile (const juce::File& f, Preset& out)
    {
        auto xml = juce::parseXML (f);
        if (xml == nullptr || ! xml->hasTagName ("RevoraPreset"))
            return false;

        const PresetValues d;   // defaults for missing attributes
        auto get = [&] (int i, float def) { return (float) xml->getDoubleAttribute (kIds[i], (double) def); };

        out.name = xml->getStringAttribute ("name", f.getFileNameWithoutExtension()).trim();
        if (out.name.isEmpty())
            return false;

        out.category = {};
        out.isFactory = false;
        out.file = f;
        out.values = { get (0, d.room), get (1, d.damp), get (2, d.width), get (3, d.tail), get (4, d.gap),
                       get (5, d.wet),  get (6, d.dry),  get (7, d.lowCut), get (8, d.highCut),
                       get (9, 0.0f) > 0.5f };
        return true;
    }
}

// =====================================================================================
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    rescanUser();
}

const std::vector<Preset>& PresetManager::getFactory()
{
    static const std::vector<Preset> factory = buildFactory();
    return factory;
}

juce::File PresetManager::getUserFolder() const
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");
   #endif
    return base.getChildFile ("Revora").getChildFile ("Presets");
}

void PresetManager::rescanUser()
{
    userPresets.clear();

    const auto folder = getUserFolder();
    if (folder.isDirectory())
    {
        for (const auto& f : folder.findChildFiles (juce::File::findFiles, false, "*.revpreset"))
        {
            Preset p;
            if (readUserFile (f, p))
                userPresets.push_back (std::move (p));
        }
    }

    std::sort (userPresets.begin(), userPresets.end(),
               [] (const Preset& a, const Preset& b) { return a.name.compareIgnoreCase (b.name) < 0; });
}

// -------------------------------------------------------------------------------------
PresetValues PresetManager::readCurrent() const
{
    float a[10];
    for (int i = 0; i < 10; ++i)
        a[i] = apvts.getRawParameterValue (kIds[i])->load();

    return { a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9] > 0.5f };
}

void PresetManager::apply (const PresetValues& v)
{
    const auto a = toArray (v);
    for (int i = 0; i < 10; ++i)
    {
        if (auto* p = apvts.getParameter (kIds[i]))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (a[(size_t) i]));
            p->endChangeGesture();
        }
    }
}

void PresetManager::setCurrent (const Preset& p)
{
    const juce::ScopedLock sl (lock);
    reference = p.values;
    currentName = p.name;
    currentCategory = p.category;
    currentUser = ! p.isFactory;
    hasCurrent = true;
}

bool PresetManager::loadFactory (int index)
{
    const auto& f = getFactory();
    if (index < 0 || index >= (int) f.size())
        return false;

    apply (f[(size_t) index].values);
    setCurrent (f[(size_t) index]);
    return true;
}

bool PresetManager::loadUser (int index)
{
    if (index < 0 || index >= (int) userPresets.size())
        return false;

    apply (userPresets[(size_t) index].values);
    setCurrent (userPresets[(size_t) index]);
    return true;
}

void PresetManager::step (int direction)
{
    rescanUser();

    const auto& fac = getFactory();
    const int nFactory = (int) fac.size();
    const int total = nFactory + (int) userPresets.size();
    if (total == 0)
        return;

    int cur = -1;
    {
        const juce::ScopedLock sl (lock);
        if (hasCurrent)
        {
            if (! currentUser)
            {
                for (int i = 0; i < nFactory; ++i)
                    if (fac[(size_t) i].name == currentName) { cur = i; break; }
            }
            else
            {
                for (int i = 0; i < (int) userPresets.size(); ++i)
                    if (userPresets[(size_t) i].name.equalsIgnoreCase (currentName)) { cur = nFactory + i; break; }
            }
        }
    }

    const int dir = direction < 0 ? -1 : 1;
    const int next = cur < 0 ? (dir > 0 ? 0 : total - 1) : (cur + dir + total) % total;

    if (next < nFactory) loadFactory (next);
    else                 loadUser (next - nFactory);
}

// -------------------------------------------------------------------------------------
bool PresetManager::saveUser (const juce::String& rawName)
{
    const auto name = rawName.trim().substring (0, 48);
    if (name.isEmpty())
        return false;

    const auto values = readCurrent();
    const auto a = toArray (values);

    juce::XmlElement xml ("RevoraPreset");
    xml.setAttribute ("version", 1);
    xml.setAttribute ("name", name);
    for (int i = 0; i < 10; ++i)
        xml.setAttribute (kIds[i], (double) a[(size_t) i]);

    const auto folder = getUserFolder();
    if (! folder.createDirectory().wasOk())
        return false;

    const auto file = folder.getChildFile (juce::File::createLegalFileName (name) + ".revpreset");
    if (! xml.writeTo (file))
        return false;

    rescanUser();

    Preset p;
    p.name = name;
    p.values = values;
    p.isFactory = false;
    p.file = file;
    setCurrent (p);
    return true;
}

bool PresetManager::deleteUser (const juce::String& name)
{
    bool deleted = false;
    for (const auto& p : userPresets)
        if (p.name.equalsIgnoreCase (name))
            deleted = p.file.deleteFile() || deleted;

    rescanUser();

    if (deleted)
    {
        const juce::ScopedLock sl (lock);
        if (currentUser && currentName.equalsIgnoreCase (name))
        {
            hasCurrent = false;
            currentUser = false;
            currentName = {};
            currentCategory = {};
        }
    }
    return deleted;
}

bool PresetManager::userExists (const juce::String& name) const
{
    for (const auto& p : userPresets)
        if (p.name.equalsIgnoreCase (name.trim()))
            return true;
    return false;
}

// -------------------------------------------------------------------------------------
juce::String PresetManager::getCurrentName() const
{
    const juce::ScopedLock sl (lock);
    return hasCurrent ? currentName : juce::String();
}

juce::String PresetManager::getCurrentCategory() const
{
    const juce::ScopedLock sl (lock);
    return hasCurrent ? currentCategory : juce::String();
}

bool PresetManager::isCurrentUser() const
{
    const juce::ScopedLock sl (lock);
    return hasCurrent && currentUser;
}

bool PresetManager::isModified() const
{
    PresetValues ref;
    {
        const juce::ScopedLock sl (lock);
        if (! hasCurrent)
            return false;
        ref = reference;
    }

    const auto cur = toArray (readCurrent());
    const auto rf  = toArray (ref);

    for (int i = 0; i < 10; ++i)
        if (auto* p = apvts.getParameter (kIds[i]))
            if (std::abs (p->convertTo0to1 (cur[(size_t) i]) - p->convertTo0to1 (rf[(size_t) i])) > 0.002f)
                return true;

    return false;
}

void PresetManager::restore (const juce::String& name, bool isUser)
{
    {
        const juce::ScopedLock sl (lock);
        hasCurrent = false;
        currentUser = false;
        currentName = {};
        currentCategory = {};
    }

    if (name.isEmpty())
        return;

    if (isUser)
    {
        rescanUser();
        for (const auto& p : userPresets)
            if (p.name.equalsIgnoreCase (name)) { setCurrent (p); return; }
    }
    else
    {
        for (const auto& p : getFactory())
            if (p.name == name) { setCurrent (p); return; }
    }
}
