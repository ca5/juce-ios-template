#include "PluginProcessor.h"
#include "PluginEditor.h"

JuceAppTemplateAudioProcessorEditor::JuceAppTemplateAudioProcessorEditor (JuceAppTemplateAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setOpaque (true);

    const auto bridgeInitScript = juce::String (
        "window.__initJuceBridge = function() {"
        "  if (!window.webkit) window.webkit = {};"
        "  if (!window.webkit.messageHandlers) window.webkit.messageHandlers = {};"
        "  window.webkit.messageHandlers.juce = {"
        "    postMessage: function(msg) {"
        "      if (window.__JUCE__ && window.__JUCE__.backend) {"
        "        window.__JUCE__.backend.emitEvent('juceBridge', msg);"
        "      }"
        "    }"
        "  };"
        "};"
        "if (document.readyState === 'loading') {"
        "  document.addEventListener('DOMContentLoaded', window.__initJuceBridge);"
        "} else {"
        "  window.__initJuceBridge();"
        "}"
    );

    auto options = juce::WebBrowserComponent::Options()
        .withNativeIntegrationEnabled()
        .withUserScript (bridgeInitScript)
        .withEventListener ("juceBridge", [this] (const juce::var& data) {
            if (data.isObject())
            {
                auto eventName = data.getProperty ("event", {}).toString();
                auto payload = data.getProperty ("payload", {});
                handleWebEvent (eventName, payload);
            }
        });

    // ui/dist フォルダの解決
    const auto distDir = getUiDistFolder();
    const auto indexPath = distDir.getChildFile ("index.html");

    // WebBrowserComponent の初期化
    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webView);

    if (indexPath.existsAsFile())
    {
        webView->goToURL (juce::URL (indexPath).toString (true));
    }
    else
    {
        // 開発時の Vite ローカルサーバーフォールバック
        webView->goToURL ("http://localhost:5173");
    }

    setSize (400, 600);
}

JuceAppTemplateAudioProcessorEditor::~JuceAppTemplateAudioProcessorEditor()
{
}

juce::File JuceAppTemplateAudioProcessorEditor::getUiDistFolder() const
{
    const auto appDir = juce::File::getSpecialLocation (juce::File::currentApplicationFile);

   #if JUCE_IOS
    // iOS Standalone (.app/ui/dist) または AUv3 Extension (.appex/ui/dist)
    auto candidate = appDir.getChildFile ("ui").getChildFile ("dist");
    if (candidate.exists())
        return candidate;

    return appDir.getChildFile ("Resources").getChildFile ("ui").getChildFile ("dist");
   #elif JUCE_MAC
    return appDir.getChildFile ("Contents").getChildFile ("Resources").getChildFile ("ui").getChildFile ("dist");
   #else
    return juce::File::getCurrentWorkingDirectory().getChildFile ("ui").getChildFile ("dist");
   #endif
}

void JuceAppTemplateAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void JuceAppTemplateAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

void JuceAppTemplateAudioProcessorEditor::handleWebEvent (const juce::String& eventName, const juce::var& payload)
{
    if (eventName == "setGain")
    {
        if (payload.isDouble() || payload.isInt())
        {
            const auto gainVal = static_cast<float> (payload);
            if (auto* param = processorRef.apvts.getParameter ("gain"))
                param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (gainVal));
        }
    }
}

void JuceAppTemplateAudioProcessorEditor::emitEventToWeb (const juce::String& eventName, const juce::var& payload)
{
    if (webView == nullptr)
        return;

    const auto jsonPayload = juce::JSON::toString (payload, true);
    const auto script = juce::String ("window.__juceReceiveEvent && window.__juceReceiveEvent('")
                      + eventName + "', " + jsonPayload + ");";

    webView->evaluateJavascript (script);
}
