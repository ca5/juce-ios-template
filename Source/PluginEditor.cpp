#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Haptics.h"

class AUv3ContainerComponent : public juce::Component
{
public:
    AUv3ContainerComponent (std::unique_ptr<juce::AudioProcessorEditor> editor,
                            const juce::String& title,
                            std::function<void()> onClose)
        : pluginEditor (std::move (editor)),
          closeCallback (std::move (onClose))
    {
        setOpaque (true);

        titleLabel.setText (title, juce::dontSendNotification);
        titleLabel.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
        titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (titleLabel);

        closeButton.setButtonText ("CLOSE [X]");
        closeButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        closeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        closeButton.onClick = [this] { if (closeCallback) closeCallback(); };
        addAndMakeVisible (closeButton);

        if (pluginEditor != nullptr)
            addAndMakeVisible (*pluginEditor);

        bringButtonsToFront();
    }

    ~AUv3ContainerComponent() override
    {
        if (pluginEditor != nullptr)
        {
            // Explicitly notify processor of editor deletion to satisfy JUCE assertions
            // and avoid crashes in ~AudioProcessorEditor and ~AudioProcessor
            pluginEditor->processor.editorBeingDeleted (pluginEditor.get());
            pluginEditor = nullptr;
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);

        // Header separator line
        g.setColour (juce::Colours::grey);
        g.drawHorizontalLine (headerY + headerHeight, 0.0f, (float) getWidth());
    }

    void childBoundsChanged (Component* child) override
    {
        if (child == pluginEditor.get() && !isResizing)
            resized();

        bringButtonsToFront();
    }

    void bringButtonsToFront()
    {
        closeButton.toFront (true);
        titleLabel.toFront (true);
    }

    void resized() override
    {
        if (isResizing) return;
        const juce::ScopedValueSetter<bool> svs (isResizing, true);

        const auto bounds = getLocalBounds();

       #if JUCE_IOS
        headerY = 54; // Safe margin below iPhone Dynamic Island / Notch
        const int bottomMargin = 34; // Safe margin above iPhone Home Indicator
       #else
        headerY = 0;
        const int bottomMargin = 0;
       #endif

        headerHeight = 44;

        closeButton.setBounds (bounds.getRight() - 100, headerY + 2, 92, 40);
        titleLabel.setBounds (12, headerY + 2, bounds.getWidth() - 120, 40);

        if (pluginEditor != nullptr)
        {
            const int editorY = headerY + headerHeight + 2;
            const int editorH = juce::jmax (50, bounds.getBottom() - editorY - bottomMargin);
            pluginEditor->setBounds (0, editorY, bounds.getWidth(), editorH);
        }

        bringButtonsToFront();
    }

private:
    std::unique_ptr<juce::AudioProcessorEditor> pluginEditor;
    std::function<void()> closeCallback;

    juce::Label titleLabel;
    juce::TextButton closeButton;

    int headerY = 0;
    int headerHeight = 44;
    bool isResizing = false;
};

JuceAppTemplateAudioProcessorEditor::JuceAppTemplateAudioProcessorEditor (JuceAppTemplateAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setOpaque (true);

    processorRef.getAUv3Manager().onPluginsChanged = [this] {
        juce::MessageManager::callAsync ([this] {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("plugins", processorRef.getAUv3Manager().getInstalledPluginsAsVar());
            emitEventToWeb ("availablePluginsUpdate", juce::var (obj));
        });
    };

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
    processorRef.getAUv3Manager().onPluginsChanged = nullptr;
    hideAUv3Overlay();
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

    if (auv3Overlay != nullptr)
        auv3Overlay->setBounds (getLocalBounds());
}

void JuceAppTemplateAudioProcessorEditor::showAUv3Overlay (int slotIndex)
{
    hideAUv3Overlay();

    auto* instance = processorRef.getAUv3Manager().getSlotInstance (slotIndex);
    if (instance == nullptr) return;

    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (instance->createEditorAndMakeActive());
    if (editor == nullptr) return;

    auv3Overlay = std::make_unique<AUv3ContainerComponent> (
        std::move (editor),
        instance->getName(),
        [this] { hideAUv3Overlay(); }
    );
    addAndMakeVisible (*auv3Overlay);
    auv3Overlay->toFront (true);
    resized();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("slotIndex", slotIndex);
    emitEventToWeb ("pluginGuiOpened", juce::var (obj));
}

void JuceAppTemplateAudioProcessorEditor::hideAUv3Overlay()
{
    if (auv3Overlay != nullptr)
    {
        removeChildComponent (auv3Overlay.get());
        auv3Overlay = nullptr;
        emitEventToWeb ("pluginGuiClosed", juce::var (new juce::DynamicObject()));
    }
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
    else if (eventName == "haptic")
    {
        juce::String styleStr = "light";
        if (payload.isObject())
            styleStr = payload.getProperty ("style", "light").toString();
        else if (payload.isString())
            styleStr = payload.toString();

        Haptics::triggerFromString (styleStr);
    }
    else if (eventName == "getAvailablePlugins")
    {
        processorRef.getAUv3Manager().scanInstalledPlugins();
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("plugins", processorRef.getAUv3Manager().getInstalledPluginsAsVar());
        emitEventToWeb ("availablePluginsUpdate", juce::var (obj));
    }
    else if (eventName == "loadPluginSlot")
    {
        int slotIndex = 0;
        juce::String ident;
        if (payload.isObject())
        {
            slotIndex = payload.getProperty ("slotIndex", 0);
            ident = payload.getProperty ("identifier", "").toString();
        }
        processorRef.getAUv3Manager().loadPluginIntoSlot (slotIndex, ident, [this] (bool) {
            juce::MessageManager::callAsync ([this] {
                auto* slotsObj = new juce::DynamicObject();
                slotsObj->setProperty ("slots", processorRef.getAUv3Manager().getSlotsStateAsVar());
                emitEventToWeb ("slotsUpdate", juce::var (slotsObj));
            });
        });
    }
    else if (eventName == "unloadPluginSlot")
    {
        int slotIndex = 0;
        if (payload.isObject())
            slotIndex = payload.getProperty ("slotIndex", 0);
        else if (payload.isInt())
            slotIndex = static_cast<int> (payload);
        processorRef.getAUv3Manager().unloadSlot (slotIndex);
        auto* slotsObj = new juce::DynamicObject();
        slotsObj->setProperty ("slots", processorRef.getAUv3Manager().getSlotsStateAsVar());
        emitEventToWeb ("slotsUpdate", juce::var (slotsObj));
    }
    else if (eventName == "togglePluginSlot")
    {
        int slotIndex = 0;
        bool enabled = true;
        if (payload.isObject())
        {
            slotIndex = payload.getProperty ("slotIndex", 0);
            enabled = payload.getProperty ("enabled", true);
        }
        processorRef.getAUv3Manager().setSlotEnabled (slotIndex, enabled);
        auto* slotsObj = new juce::DynamicObject();
        slotsObj->setProperty ("slots", processorRef.getAUv3Manager().getSlotsStateAsVar());
        emitEventToWeb ("slotsUpdate", juce::var (slotsObj));
    }
    else if (eventName == "setPluginDryWet")
    {
        int slotIndex = 0;
        float dryWet = 1.0f;
        if (payload.isObject())
        {
            slotIndex = payload.getProperty ("slotIndex", 0);
            dryWet = static_cast<float> (payload.getProperty ("dryWet", 1.0));
        }
        processorRef.getAUv3Manager().setSlotDryWet (slotIndex, dryWet);
        auto* slotsObj = new juce::DynamicObject();
        slotsObj->setProperty ("slots", processorRef.getAUv3Manager().getSlotsStateAsVar());
        emitEventToWeb ("slotsUpdate", juce::var (slotsObj));
    }
    else if (eventName == "openPluginGUI")
    {
        int slotIndex = 0;
        if (payload.isObject())
            slotIndex = payload.getProperty ("slotIndex", 0);
        else if (payload.isInt())
            slotIndex = static_cast<int> (payload);
        showAUv3Overlay (slotIndex);
    }
    else if (eventName == "closePluginGUI")
    {
        hideAUv3Overlay();
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
