#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double kMaxSourceSeconds = 60.0;

    // parameters that require re-rendering when changed
    const char* const kRenderParams[] = { "room", "damp", "width", "tail", "gap",
                                          "wet", "dry", "lowcut", "highcut", "swellOnly" };

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using P  = juce::AudioParameterFloat;
        using B  = juce::AudioParameterBool;
        using NR = juce::NormalisableRange<float>;

        using Attr = juce::AudioParameterFloatAttributes;

        const auto pct = Attr()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });
        const auto sec = Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " s"; });
        const auto ms  = Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; });
        const auto hz  = Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " Hz"; });
        const auto db  = Attr().withStringFromValueFunction ([] (float v, int)
                         { return v <= -59.9f ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; });

        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<P> (juce::ParameterID { "room", 1 },    "Room size",       NR (0.0f, 1.0f, 0.001f), 0.8f, pct));
        l.add (std::make_unique<P> (juce::ParameterID { "damp", 1 },    "Damping",         NR (0.0f, 1.0f, 0.001f), 0.5f, pct));
        l.add (std::make_unique<P> (juce::ParameterID { "width", 1 },   "Width",           NR (0.0f, 1.0f, 0.001f), 1.0f, pct));
        l.add (std::make_unique<P> (juce::ParameterID { "tail", 1 },    "Swell length",    NR (0.2f, 8.0f, 0.01f, 0.5f), 2.0f, sec));
        l.add (std::make_unique<P> (juce::ParameterID { "gap", 1 },     "Gap before dry",  NR (0.0f, 500.0f, 1.0f), 0.0f, ms));
        l.add (std::make_unique<P> (juce::ParameterID { "wet", 1 },     "Wet level",       NR (-60.0f, 12.0f, 0.1f), 0.0f, db));
        l.add (std::make_unique<P> (juce::ParameterID { "dry", 1 },     "Dry level",       NR (-60.0f, 12.0f, 0.1f), 0.0f, db));
        l.add (std::make_unique<P> (juce::ParameterID { "lowcut", 1 },  "Low cut",         NR (20.0f, 2000.0f, 1.0f, 0.4f), 80.0f, hz));
        l.add (std::make_unique<P> (juce::ParameterID { "highcut", 1 }, "High cut",        NR (1000.0f, 20000.0f, 1.0f, 0.4f), 12000.0f, hz));
        l.add (std::make_unique<B> (juce::ParameterID { "swellOnly", 1 }, "Swell only", false));
        l.add (std::make_unique<B> (juce::ParameterID { "passthru", 1 },  "Monitor input", true));
        l.add (std::make_unique<B> (juce::ParameterID { "loop", 1 },      "Loop", false));
        return l;
    }

    // ---------------------------------------------------------------------------------
    juce::AudioBuffer<float> resampleStereo (const juce::AudioBuffer<float>& in, double fromSr, double toSr)
    {
        if (std::abs (fromSr - toSr) < 1.0e-6)
            return in;

        const double ratio = fromSr / toSr;
        const int newLen = juce::jmax (1, (int) std::ceil (in.getNumSamples() / ratio));

        // pad the input so the interpolator never reads past the end
        const int pad = (int) std::ceil (ratio) + 8;
        juce::AudioBuffer<float> padded (2, in.getNumSamples() + pad);
        padded.clear();
        for (int ch = 0; ch < 2; ++ch)
            padded.copyFrom (ch, 0, in, ch, 0, in.getNumSamples());

        juce::AudioBuffer<float> out (2, newLen);
        out.clear();
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.process (ratio, padded.getReadPointer (ch), out.getWritePointer (ch), newLen);
        }
        return out;
    }

    // ---------------------------------------------------------------------------------
    struct RenderSettings
    {
        float room, damp, width, tailSec, gapMs, wetDb, dryDb, lowCut, highCut;
        bool swellOnly;
        double sr;
    };

    /*  Reverse reverb:
        1. reverse the source and append `tail` seconds of silence
        2. run it through a (wet only) reverb
        3. fade the reverb tail out
        4. reverse everything again  -> the reverb "swells" up to the start of the original
        5. mix with the original, which is placed right after the swell                     */
    juce::AudioBuffer<float> renderReverseReverb (const juce::AudioBuffer<float>& src,
                                                  const RenderSettings& s,
                                                  int& dryOffsetOut)
    {
        const int n      = src.getNumSamples();
        const int tail   = juce::jmax (1, (int) std::lround (s.tailSec * s.sr));
        const int gap    = juce::jmax (0, (int) std::lround (s.gapMs * 0.001 * s.sr));
        const int wetLen = n + tail;
        const int total  = wetLen + gap;

        // 1) reversed source + silent tail
        juce::AudioBuffer<float> x (2, wetLen);
        x.clear();
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* in = src.getReadPointer (ch);
            float* o = x.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
                o[i] = in[n - 1 - i];
        }

        // 2) reverb, wet only
        juce::Reverb reverb;
        reverb.setSampleRate (s.sr);
        juce::Reverb::Parameters p;
        p.roomSize   = s.room;
        p.damping    = s.damp;
        p.wetLevel   = 1.0f;
        p.dryLevel   = 0.0f;
        p.width      = s.width;
        p.freezeMode = 0.0f;
        reverb.setParameters (p);
        reverb.processStereo (x.getWritePointer (0), x.getWritePointer (1), wetLen);

        // 3) shape the tail region (cosine fade-out) so the swell starts from silence
        for (int ch = 0; ch < 2; ++ch)
        {
            float* o = x.getWritePointer (ch);
            for (int i = 0; i < tail; ++i)
            {
                const float prog = (float) i / (float) tail;
                o[n + i] *= 0.5f * (1.0f + std::cos (juce::MathConstants<float>::pi * prog));
            }
        }

        // 4) reverse back -> wet signal. The original's first sample lands at index `tail`.
        juce::AudioBuffer<float> w (2, total);
        w.clear();
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* in = x.getReadPointer (ch);
            float* o = w.getWritePointer (ch);
            for (int i = 0; i < wetLen; ++i)
                o[i] = in[wetLen - 1 - i];
        }

        if (s.swellOnly)   // keep only the part that comes BEFORE the dry sound
        {
            const int xf = juce::jmax (1, (int) (0.01 * s.sr));
            for (int ch = 0; ch < 2; ++ch)
            {
                float* o = w.getWritePointer (ch);
                for (int i = tail; i < total; ++i)
                {
                    const int k = i - tail;
                    o[i] *= k < xf ? 1.0f - (float) k / (float) xf : 0.0f;
                }
            }
        }

        // tone shaping of the wet signal
        const double hc = juce::jmin ((double) s.highCut, s.sr * 0.45);
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::IIRFilter hp, lp;
            hp.setCoefficients (juce::IIRCoefficients::makeHighPass (s.sr, (double) s.lowCut));
            lp.setCoefficients (juce::IIRCoefficients::makeLowPass  (s.sr, hc));
            hp.processSamples (w.getWritePointer (ch), total);
            lp.processSamples (w.getWritePointer (ch), total);
        }

        // 5) level matching: wet peak = 0.7 x source peak at 0 dB, then user gains
        const float pSrc = src.getMagnitude (0, n);
        const float pWet = w.getMagnitude (0, total);
        const float wetGain = pWet > 1.0e-6f
                                ? 0.7f * pSrc / pWet * juce::Decibels::decibelsToGain (s.wetDb, -60.0f)
                                : 0.0f;
        const float dryGain = juce::Decibels::decibelsToGain (s.dryDb, -60.0f);

        juce::AudioBuffer<float> out (2, total);
        out.clear();
        for (int ch = 0; ch < 2; ++ch)
        {
            out.addFrom (ch, 0,         w,   ch, 0, total, wetGain);
            out.addFrom (ch, tail + gap, src, ch, 0, n,     dryGain);
        }

        const float peak = out.getMagnitude (0, total);
        if (peak > 0.98f)
            out.applyGain (0.98f / peak);

        dryOffsetOut = tail + gap;
        return out;
    }

    bool writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
    {
        f.getParentDirectory().createDirectory();
        f.deleteFile();

        std::unique_ptr<juce::FileOutputStream> stream (f.createOutputStream());
        if (stream == nullptr)
            return false;

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (stream.get(), sr, (unsigned int) b.getNumChannels(), 24, {}, 0));
        if (writer == nullptr)
            return false;

        stream.release();   // the writer owns the stream now
        return writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }

    juce::File exportFileFor (const juce::String& name)
    {
        return juce::File::getSpecialLocation (juce::File::tempDirectory)
                   .getChildFile ("Revora")
                   .getChildFile (juce::File::createLegalFileName (name) + "_revora.wav");
    }
}

// =====================================================================================
ReverseReverbProcessor::ReverseReverbProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()),
      presets (apvts)
{
    formatManager.registerBasicFormats();

    loopParam     = apvts.getRawParameterValue ("loop");
    passthruParam = apvts.getRawParameterValue ("passthru");

    for (auto* id : kRenderParams)
        apvts.addParameterListener (id, this);

    startTimerHz (10);
}

ReverseReverbProcessor::~ReverseReverbProcessor()
{
    stopTimer();
    for (auto* id : kRenderParams)
        apvts.removeParameterListener (id, this);

    if (renderThread.joinable())
        renderThread.join();
}

bool ReverseReverbProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ReverseReverbProcessor::prepareToPlay (double sampleRate, int)
{
    captureBuffer.setSize (2, (int) (sampleRate * kMaxSourceSeconds), false, true, false);
    capturePos = 0;
    captureState = 0;

    if (std::abs (sampleRate - currentSr.load()) > 1.0e-6)
    {
        {
            const juce::ScopedLock sl (dataLock);
            if (source != nullptr)
                source = std::make_shared<juce::AudioBuffer<float>> (resampleStereo (*source, currentSr.load(), sampleRate));
        }
        currentSr = sampleRate;
        lastChangeMs = juce::Time::getMillisecondCounter();
        dirty = true;
    }
}

// =====================================================================================
void ReverseReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    // --- capture incoming audio -------------------------------------------------------
    const int cs = captureState.load();
    if (cs == 1)
    {
        int pos = capturePos.load();
        const int toCopy = juce::jmax (0, juce::jmin (n, captureBuffer.getNumSamples() - pos));
        for (int ch = 0; ch < 2; ++ch)
            captureBuffer.copyFrom (ch, pos, buffer.getReadPointer (juce::jmin (ch, numCh - 1)), toCopy);
        pos += toCopy;
        capturePos = pos;
        if (pos >= captureBuffer.getNumSamples())
            captureState = 3;
    }
    else if (cs == 2)
    {
        captureState = 3;
    }

    if (passthruParam->load() < 0.5f)
        buffer.clear();

    // --- play the rendered result -----------------------------------------------------
    if (playing.load())
    {
        juce::SpinLock::ScopedTryLockType lock (renderedLock);
        if (lock.isLocked() && rendered != nullptr)
        {
            const auto& r = *rendered;
            const int len = r.getNumSamples();
            const bool loop = loopParam->load() > 0.5f;
            int pos = playPos.load();

            for (int i = 0; i < n; ++i)
            {
                if (pos >= len)
                {
                    if (loop) pos = 0;
                    else { playing = false; break; }
                }
                for (int ch = 0; ch < numCh; ++ch)
                    buffer.getWritePointer (ch)[i] += r.getSample (juce::jmin (ch, 1), pos);
                ++pos;
            }
            playPos = pos;
        }
    }
}

// =====================================================================================
void ReverseReverbProcessor::timerCallback()
{
    const int cs = captureState.load();

    if (cs == 3)
    {
        finalizeCapture();
    }
    else if (cs == 2 && juce::Time::getMillisecondCounter() - stopRequestedAt.load() > 500)
    {
        // host is not calling processBlock (transport stopped) -> finish anyway
        int expected = 2;
        captureState.compare_exchange_strong (expected, 3);
    }

    const auto now = juce::Time::getMillisecondCounter();
    if (dirty.load() && now - lastChangeMs.load() > 120 && ! rendering.load() && hasSource())
    {
        dirty = false;
        startRender();
    }
}

void ReverseReverbProcessor::parameterChanged (const juce::String&, float)
{
    lastChangeMs = juce::Time::getMillisecondCounter();
    dirty = true;
}

void ReverseReverbProcessor::setSource (juce::AudioBuffer<float>&& b, const juce::String& name, const juce::String& path)
{
    std::shared_ptr<const juce::AudioBuffer<float>> ptr = std::make_shared<juce::AudioBuffer<float>> (std::move (b));
    {
        const juce::ScopedLock sl (dataLock);
        source = ptr;
        sourceName = name;
        sourcePath = path;
    }
    lastChangeMs = juce::Time::getMillisecondCounter() - 1000u;
    dirty = true;
}

bool ReverseReverbProcessor::loadFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0
        || reader->lengthInSamples > (juce::int64) (kMaxSourceSeconds * reader->sampleRate))
        return false;

    const int len = (int) reader->lengthInSamples;
    juce::AudioBuffer<float> tmp (2, len);
    tmp.clear();
    reader->read (&tmp, 0, len, 0, true, true);
    if (reader->numChannels == 1)
        tmp.copyFrom (1, 0, tmp, 0, 0, len);

    setSource (resampleStereo (tmp, reader->sampleRate, currentSr.load()),
               file.getFileNameWithoutExtension(), file.getFullPathName());
    return true;
}

void ReverseReverbProcessor::startCapture()
{
    if (captureState.load() == 0)
    {
        capturePos = 0;
        captureState = 1;
    }
}

void ReverseReverbProcessor::stopCapture()
{
    int expected = 1;
    if (captureState.compare_exchange_strong (expected, 2))
        stopRequestedAt = juce::Time::getMillisecondCounter();
}

void ReverseReverbProcessor::finalizeCapture()
{
    const int len = capturePos.load();
    if (len > 256)
    {
        juce::AudioBuffer<float> b (2, len);
        for (int ch = 0; ch < 2; ++ch)
            b.copyFrom (ch, 0, captureBuffer, ch, 0, len);
        setSource (std::move (b), "Captured from DAW", {});
    }
    captureState = 0;
}

void ReverseReverbProcessor::startRender()
{
    std::shared_ptr<const juce::AudioBuffer<float>> src;
    juce::String name;
    {
        const juce::ScopedLock sl (dataLock);
        src = source;
        name = sourceName;
    }
    if (src == nullptr)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    RenderSettings s;
    s.room      = get ("room");
    s.damp      = get ("damp");
    s.width     = get ("width");
    s.tailSec   = get ("tail");
    s.gapMs     = get ("gap");
    s.wetDb     = get ("wet");
    s.dryDb     = get ("dry");
    s.lowCut    = get ("lowcut");
    s.highCut   = get ("highcut");
    s.swellOnly = get ("swellOnly") > 0.5f;
    s.sr        = currentSr.load();

    if (renderThread.joinable())
        renderThread.join();

    rendering = true;

    renderThread = std::thread ([this, src, s, name]
    {
        int dryOffset = 0;
        auto out = std::make_shared<juce::AudioBuffer<float>> (renderReverseReverb (*src, s, dryOffset));

        const auto file = exportFileFor (name);
        writeWav (*out, s.sr, file);

        std::shared_ptr<const juce::AudioBuffer<float>> old;
        {
            const juce::SpinLock::ScopedLockType sl (renderedLock);
            old = rendered;
            rendered = out;
            renderedFile = file;
        }
        dryOffsetSamples = dryOffset;
        ++renderVersion;
        rendering = false;
        // `old` is released here, outside the lock
    });
}

// =====================================================================================
void ReverseReverbProcessor::togglePlay()
{
    if (playing.load())
    {
        playing = false;
    }
    else
    {
        playPos = 0;
        playing = true;
    }
}

float ReverseReverbProcessor::getPlayheadFraction() const
{
    auto r = getRendered();
    if (r == nullptr || r->getNumSamples() == 0)
        return 0.0f;
    return juce::jlimit (0.0f, 1.0f, (float) playPos.load() / (float) r->getNumSamples());
}

bool ReverseReverbProcessor::hasSource() const
{
    const juce::ScopedLock sl (dataLock);
    return source != nullptr;
}

juce::String ReverseReverbProcessor::getSourceName() const
{
    const juce::ScopedLock sl (dataLock);
    return sourceName;
}

std::shared_ptr<const juce::AudioBuffer<float>> ReverseReverbProcessor::getRendered() const
{
    auto& self = const_cast<ReverseReverbProcessor&> (*this);
    const juce::SpinLock::ScopedLockType sl (self.renderedLock);
    return rendered;
}

juce::File ReverseReverbProcessor::getRenderedFile() const
{
    auto& self = const_cast<ReverseReverbProcessor&> (*this);
    const juce::SpinLock::ScopedLockType sl (self.renderedLock);
    return renderedFile;
}

// =====================================================================================
void ReverseReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        const juce::ScopedLock sl (dataLock);
        state.setProperty ("sourcePath", sourcePath, nullptr);
    }
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    state.setProperty ("presetUser", presets.isCurrentUser(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ReverseReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (vt);
            presets.restore (vt.getProperty ("presetName").toString(), (bool) vt.getProperty ("presetUser", false));

            const auto path = vt.getProperty ("sourcePath").toString();
            if (path.isNotEmpty() && juce::File (path).existsAsFile())
                loadFile (juce::File (path));
        }
    }
}

juce::AudioProcessorEditor* ReverseReverbProcessor::createEditor()
{
    return new ReverseReverbEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverseReverbProcessor();
}
