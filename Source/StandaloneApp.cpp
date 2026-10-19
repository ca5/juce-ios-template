/*
  ==============================================================================

    StandaloneApp.cpp
    Part of JUCE iOS App Template

    Implements a custom Standalone Application to fully control the UI
    and suppress the mandatory "Audio input is muted to avoid feedback loop"
    warning banner provided by the default JUCE StandaloneFilterWindow.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginEditor.h"

#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

class CustomStandaloneWindow : public juce::DocumentWindow
{
public:
    CustomStandaloneWindow (const juce::String& name,
                            juce::Colour backgroundColour,
                            juce::PropertySet* settingsToUse)
#if JUCE_IOS
        : DocumentWindow (name, backgroundColour, 0)
#else
        : DocumentWindow (name, backgroundColour, DocumentWindow::allButtons)
#endif
    {
#if JUCE_IOS
        setUsingNativeTitleBar (false);
        setTitleBarHeight (0);
        setBackgroundColour (juce::Colours::black);
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colours::black);
        setFullScreen (true);

        if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            setBounds (display->userArea);
#else
        setUsingNativeTitleBar (true);
#endif

        // 1. Initialize StandalonePluginHolder
        pluginHolder.reset (new juce::StandalonePluginHolder (settingsToUse));

        // 2. Suppress feedback loop warning and configure audio pass-through
        pluginHolder->getMuteInputValue().setValue (true);

        // 3. Audio Device Setup (0 inputs, 2 outputs to avoid microphone entitlement crash on iOS)
        auto desc = juce::AudioDeviceManager::AudioDeviceSetup();
        pluginHolder->deviceManager.getAudioDeviceSetup (desc);
        desc.inputChannels.clear();
        desc.outputChannels.setBit (0);
        desc.outputChannels.setBit (1);
        pluginHolder->deviceManager.initialise (0, 2, nullptr, true, juce::String(), &desc);

        // 4. Create and attach Plugin Editor
        if (auto* processor = pluginHolder->processor.get())
        {
            if (auto* editor = processor->createEditorIfNeeded())
            {
#if JUCE_IOS
                setContentOwned (editor, false);
#else
                setContentOwned (editor, true);
#endif
            }
        }

        setResizable (true, true);
        setVisible (true);
    }

    ~CustomStandaloneWindow() override
    {
        setContentOwned (nullptr, true);
        pluginHolder = nullptr;
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;
};

//==============================================================================
class JuceAppTemplateStandaloneApp : public juce::JUCEApplication
{
public:
    JuceAppTemplateStandaloneApp() {}

    const juce::String getApplicationName() override              { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override           { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override                    { return true; }
    void anotherInstanceStarted (const juce::String&) override    {}

    void initialise (const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = getApplicationName();
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName          = getApplicationName();
        options.storageFormat       = juce::PropertiesFile::storeAsXML;

        settings.setStorageParameters (options);

        if (auto* props = settings.getUserSettings())
            props->setValue ("muteAudioInput", true);

        mainWindow.reset (new CustomStandaloneWindow (getApplicationName(),
                                                      juce::Colours::black,
                                                      settings.getUserSettings()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    juce::ApplicationProperties settings;
    std::unique_ptr<CustomStandaloneWindow> mainWindow;
};

#endif // JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

#if JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP
START_JUCE_APPLICATION (JuceAppTemplateStandaloneApp)
#endif
