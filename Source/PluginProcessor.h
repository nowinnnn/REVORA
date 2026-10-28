#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <thread>
#include "PresetManager.h"

class ReverseReverbProcessor final : public juce::AudioProcessor,
                                     private juce::Timer,
                                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    ReverseReverbProcessor();
    ~ReverseReverbProcessor() override;

    // ---- AudioProcessor ----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Revora"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- API used by the editor (message thread) ----
    bool loadFile (const juce::File& file);      // drag & drop / open
    void startCapture();                         // record input coming from the DAW
    void stopCapture();
    bool isCapturing() const { return captureState.load() == 1; }

    void togglePlay();
    bool isPlaying() const { return playing.load(); }
    float getPlayheadFraction() const;

    bool hasSource() const;
    bool isRendering() const { return rendering.load(); }
    juce::String getSourceName() const;

    std::shared_ptr<const juce::AudioBuffer<float>> getRendered() const;
    int getRenderVersion() const { return renderVersion.load(); }
    int getDryOffsetSamples() const { return dryOffsetSamples.load(); }
    juce::File getRenderedFile() const;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;      // factory + user preset library (message thread)

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void setSource (juce::AudioBuffer<float>&& stereoBuffer, const juce::String& name, const juce::String& path);
    void finalizeCapture();
    void startRender();

    juce::AudioFormatManager formatManager;

    // source audio (always stereo, always at currentSr)
    mutable juce::CriticalSection dataLock;
    std::shared_ptr<const juce::AudioBuffer<float>> source;
    juce::String sourceName, sourcePath;
    std::atomic<double> currentSr { 44100.0 };

    // rendered result (read by the audio thread through a try-lock)
    juce::SpinLock renderedLock;
    std::shared_ptr<const juce::AudioBuffer<float>> rendered;
    juce::File renderedFile;
    std::atomic<int> dryOffsetSamples { 0 };
    std::atomic<int> renderVersion { 0 };

    // playback
    std::atomic<bool> playing { false };
    std::atomic<int> playPos { 0 };

    // capture from the DAW: 0 idle, 1 recording, 2 stop requested, 3 finished
    juce::AudioBuffer<float> captureBuffer;
    std::atomic<int> captureState { 0 };
    std::atomic<int> capturePos { 0 };
    std::atomic<juce::uint32> stopRequestedAt { 0 };

    // rendering
    std::atomic<bool> dirty { false };
    std::atomic<bool> rendering { false };
    std::atomic<juce::uint32> lastChangeMs { 0 };
    std::thread renderThread;

    std::atomic<float>* loopParam = nullptr;
    std::atomic<float>* passthruParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseReverbProcessor)
};
