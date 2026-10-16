#pragma once

#include "PluginProcessor.h"
#include <array>
#include <memory>

namespace ht { class HTLookAndFeel; class MeterBar; class ScopeView; class SampleView; }

class HardwareTrapEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HardwareTrapEditor (HardwareTrapProcessor&);
    ~HardwareTrapEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setMode (bool sampleLab);
    void refreshPresets();
    void promptSavePreset();
    void loadSampleFile (const juce::File&);
    void chooseSample();
    void chooseExport();
    void drawStatic (juce::Graphics&);
    juce::Rectangle<int> displayPanel() const { return { 20, 76, 880, 280 }; }
    juce::Rectangle<int> knobPanel() const    { return { 20, 366, 880, 226 }; }

    HardwareTrapProcessor& proc;
    std::unique_ptr<ht::HTLookAndFeel> lnf;

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        const char* name = ""; const char* sub = "";
    };
    std::array<Knob, 6> knobs;

    juce::TextButton liveTab { "LIVE MODE" }, labTab { "SAMPLE LAB" };
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "SAVE" };
    juce::ComboBox presetBox;

    juce::TextButton loadButton { "LOAD WAV" }, playButton { "PLAY" }, loopButton { "LOOP" }, exportButton { "EXPORT WAV" };

    std::unique_ptr<ht::MeterBar> meterIn, meterOut;
    std::unique_ptr<ht::ScopeView> scopeView;
    std::unique_ptr<ht::SampleView> sampleView;

    bool sampleMode = false;
    float ledSignal = 0.f, ledMove = 0.f, ledGlitch = 0.f, ledLimit = 0.f;
    juce::String statusText, lastPresetName;
    juce::Image staticLayer;
    float staticScale = 0.f;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardwareTrapEditor)
};
