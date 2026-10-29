#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class AUv3ContainerComponent;

class JuceAppTemplateAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit JuceAppTemplateAudioProcessorEditor (JuceAppTemplateAudioProcessor&);
    ~JuceAppTemplateAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // JavaScript からの関数呼び出しハンドラ
    void handleWebEvent (const juce::String& eventName, const juce::var& payload);

    // JavaScript へイベントを発行
    void emitEventToWeb (const juce::String& eventName, const juce::var& payload);

    // Native AUv3 UI presentation overlay
    void showAUv3Overlay (int slotIndex);
    void hideAUv3Overlay();

private:
    JuceAppTemplateAudioProcessor& processorRef;

    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<AUv3ContainerComponent> auv3Overlay;
    juce::File getUiDistFolder() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceAppTemplateAudioProcessorEditor)
};
