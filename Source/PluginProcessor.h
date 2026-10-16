#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <array>
#include <memory>
#include "Engine.h"

namespace ht
{
namespace ParamID
{
    inline constexpr const char* destroy  = "destroy";
    inline constexpr const char* movement = "movement";
    inline constexpr const char* texture  = "texture";
    inline constexpr const char* unstable = "unstable";
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* output   = "output";
}

struct SampleData
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::File file;
};

struct FactoryPreset { const char* name; float destroy, movement, texture, unstable, mix, outputDb; };
}

class HardwareTrapProcessor : public juce::AudioProcessor
{
public:
    HardwareTrapProcessor();
    ~HardwareTrapProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HARDWARETRAP"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- medidores / indicadores (audio -> UI) ----
    static constexpr int scopeSize = 512;
    std::atomic<float> inputPeak { 0.f }, outputPeak { 0.f };
    std::atomic<float> lfoDisplay { 0.f }, glitchFlag { 0.f }, limiterFlag { 0.f };
    std::array<std::atomic<float>, scopeSize> scope;
    std::atomic<int> scopeWrite { 0 };

    // ---- SAMPLE LAB ----
    bool loadSample (const juce::File& file, juce::String& error);
    std::shared_ptr<const ht::SampleData> getSample() const;
    std::atomic<int>   sampleVersion { 0 };
    std::atomic<float> regionStart { 0.f }, regionEnd { 1.f };    // normalizado 0..1
    std::atomic<bool>  previewPlaying { false }, previewLoop { true };
    std::atomic<float> previewPosNorm { 0.f };
    void startPreview();
    void stopPreview() { previewPlaying = false; }
    bool exportRegion (const juce::File& destination, juce::String& error);

    // ---- presets ----
    static const std::array<ht::FactoryPreset, 8>& factoryPresets();
    juce::StringArray getPresetNames() const;
    void loadPreset (int index);
    bool saveUserPreset (const juce::String& name);
    juce::String getCurrentPresetName() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    ht::Params currentParams() const;
    void applyParams (float d, float m, float t, float u, float mx, float outDb);
    void renderPreview (juce::AudioBuffer<float>&, int numSamples, int numChannels);
    juce::Array<juce::File> userPresetFiles() const;
    static juce::File presetFolder();

    std::atomic<float>* pDestroy = nullptr;
    std::atomic<float>* pMovement = nullptr;
    std::atomic<float>* pTexture = nullptr;
    std::atomic<float>* pUnstable = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;

    ht::Engine engine;
    double currentSampleRate = 44100.0;
    int scopeCounter = 0;
    float scopeAccum = 0.f;

    juce::AudioFormatManager formatManager;
    mutable juce::SpinLock sampleLock;
    std::shared_ptr<ht::SampleData> sample;
    double playPos = 0.0;
    std::atomic<bool> restartPreview { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardwareTrapProcessor)
};
