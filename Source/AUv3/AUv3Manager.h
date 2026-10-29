#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <memory>
#include <functional>

struct AUv3Slot
{
    int slotId = 1;
    std::unique_ptr<juce::AudioPluginInstance> instance;
    juce::PluginDescription desc;
    bool enabled = true;
    float dryWet = 1.0f; // 0.0 to 1.0
};

class AUv3Manager
{
public:
    AUv3Manager();
    ~AUv3Manager();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    // Query installed AUv3 effects on iOS
    void scanInstalledPlugins();
    juce::var getInstalledPluginsAsVar() const;

    // Slot controls (0, 1, 2)
    static constexpr int numSlots = 3;
    void loadPluginIntoSlot (int slotIndex, const juce::String& identifier, std::function<void (bool)> completion = nullptr);
    void unloadSlot (int slotIndex);
    void setSlotEnabled (int slotIndex, bool enabled);
    bool isSlotEnabled (int slotIndex) const;
    void setSlotDryWet (int slotIndex, float dryWet);
    juce::var getSlotsStateAsVar() const;

    // Audio stream processing
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // Access instance for native GUI presentation
    juce::AudioPluginInstance* getSlotInstance (int slotIndex);
    juce::AudioProcessorEditor* createSlotEditor (int slotIndex);

    std::function<void()> onPluginsChanged;

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    std::array<AUv3Slot, numSlots> slots;
    juce::SpinLock processLock;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    void* notificationObserver = nullptr;
};
