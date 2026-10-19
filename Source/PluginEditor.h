#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

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

private:
    JuceAppTemplateAudioProcessor& processorRef;

    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::File getUiDistFolder() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceAppTemplateAudioProcessorEditor)
};
