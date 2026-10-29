#include "AUv3Manager.h"
#include <cmath>

#if !JUCE_IOS && !JUCE_MAC

AUv3Manager::AUv3Manager()
{
    juce::addDefaultFormatsToManager (formatManager);

    for (int i = 0; i < numSlots; ++i)
    {
        slots[i].slotId = i + 1;
        slots[i].enabled = true;
        slots[i].dryWet = 1.0f;
    }

    scanInstalledPlugins();
}

AUv3Manager::~AUv3Manager()
{
    releaseResources();
}

void AUv3Manager::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::SpinLock::ScopedLockType sl (processLock);
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    for (auto& slot : slots)
    {
        if (slot.instance != nullptr)
            slot.instance->prepareToPlay (sampleRate, samplesPerBlock);
    }
}

void AUv3Manager::releaseResources()
{
    const juce::SpinLock::ScopedLockType sl (processLock);
    for (auto& slot : slots)
    {
        if (slot.instance != nullptr)
            slot.instance->releaseResources();
    }
}

void AUv3Manager::scanInstalledPlugins()
{
    knownPluginList.clear();

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat (i);
        if (format == nullptr) continue;

        const auto searchPaths = format->searchPathsForPlugins ({}, false, true);
        for (const auto& pathOrId : searchPaths)
        {
            juce::OwnedArray<juce::PluginDescription> found;
            format->findAllTypesForFile (found, pathOrId);
            for (auto* desc : found)
                knownPluginList.addType (*desc);
        }
    }
}

juce::var AUv3Manager::getInstalledPluginsAsVar() const
{
    juce::Array<juce::var> pluginListVar;

    for (const auto& desc : knownPluginList.getTypes())
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("name", desc.name);
        obj->setProperty ("descriptiveName", desc.descriptiveName);
        obj->setProperty ("manufacturer", desc.manufacturerName);
        obj->setProperty ("category", desc.category);
        obj->setProperty ("identifier", desc.fileOrIdentifier);
        obj->setProperty ("isInstrument", desc.isInstrument);
        pluginListVar.add (juce::var (obj));
    }

    return juce::var (pluginListVar);
}

void AUv3Manager::loadPluginIntoSlot (int slotIndex, const juce::String& identifier, std::function<void (bool)> completion)
{
    if (slotIndex < 0 || slotIndex >= numSlots)
    {
        if (completion) completion (false);
        return;
    }

    unloadSlot (slotIndex);

    juce::PluginDescription targetDesc;
    bool found = false;

    for (const auto& desc : knownPluginList.getTypes())
    {
        if (desc.fileOrIdentifier == identifier)
        {
            targetDesc = desc;
            found = true;
            break;
        }
    }

    if (! found)
    {
        targetDesc.fileOrIdentifier = identifier;
        targetDesc.pluginFormatName = "AudioUnit";
    }

    formatManager.createPluginInstanceAsync (targetDesc, currentSampleRate, currentBlockSize,
        [this, slotIndex, targetDesc, completion] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            if (instance != nullptr)
            {
                juce::AudioProcessor::BusesLayout layout = instance->getBusesLayout();
                if (instance->getBusCount (true) > 0)
                    layout.inputBuses.set (0, juce::AudioChannelSet::stereo());
                if (instance->getBusCount (false) > 0)
                    layout.outputBuses.set (0, juce::AudioChannelSet::stereo());

                instance->setBusesLayout (layout);
                instance->prepareToPlay (currentSampleRate, currentBlockSize);

                {
                    const juce::SpinLock::ScopedLockType sl (processLock);
                    slots[slotIndex].instance = std::move (instance);
                    slots[slotIndex].desc = targetDesc;
                    slots[slotIndex].enabled = true;
                    slots[slotIndex].dryWet = 1.0f;
                }

                if (completion) completion (true);
            }
            else
            {
                juce::Logger::writeToLog ("Failed to instantiate AUv3: " + error);
                if (completion) completion (false);
            }
        });
}

void AUv3Manager::unloadSlot (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= numSlots) return;

    std::unique_ptr<juce::AudioPluginInstance> oldInstance;
    {
        const juce::SpinLock::ScopedLockType sl (processLock);
        oldInstance = std::move (slots[slotIndex].instance);
        slots[slotIndex].desc = {};
    }

    if (oldInstance != nullptr)
    {
        oldInstance->releaseResources();
        oldInstance = nullptr;
    }
}

void AUv3Manager::setSlotEnabled (int slotIndex, bool enabled)
{
    if (slotIndex >= 0 && slotIndex < numSlots)
        slots[slotIndex].enabled = enabled;
}

bool AUv3Manager::isSlotEnabled (int slotIndex) const
{
    if (slotIndex >= 0 && slotIndex < numSlots)
        return slots[slotIndex].enabled;
    return false;
}

void AUv3Manager::setSlotDryWet (int slotIndex, float dryWet)
{
    if (slotIndex >= 0 && slotIndex < numSlots)
        slots[slotIndex].dryWet = juce::jlimit (0.0f, 1.0f, dryWet);
}

juce::var AUv3Manager::getSlotsStateAsVar() const
{
    juce::Array<juce::var> arr;
    for (int i = 0; i < numSlots; ++i)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("slotIndex", i);
        obj->setProperty ("isLoaded", slots[i].instance != nullptr);
        obj->setProperty ("enabled", slots[i].enabled);
        obj->setProperty ("dryWet", static_cast<double> (slots[i].dryWet));

        if (slots[i].instance != nullptr)
        {
            obj->setProperty ("name", slots[i].desc.name);
            obj->setProperty ("manufacturer", slots[i].desc.manufacturerName);
            obj->setProperty ("identifier", slots[i].desc.fileOrIdentifier);
        }
        else
        {
            obj->setProperty ("name", "");
            obj->setProperty ("manufacturer", "");
            obj->setProperty ("identifier", "");
        }
        arr.add (juce::var (obj));
    }
    return juce::var (arr);
}

void AUv3Manager::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const juce::SpinLock::ScopedLockType sl (processLock);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    juce::AudioBuffer<float> dryBuffer;
    bool hasDryBuffer = false;

    for (int i = 0; i < numSlots; ++i)
    {
        auto& slot = slots[i];
        if (slot.instance == nullptr || ! slot.enabled)
            continue;

        if (slot.dryWet < 0.999f && ! hasDryBuffer)
        {
            dryBuffer.setSize (numChannels, numSamples, false, false, true);
            for (int ch = 0; ch < numChannels; ++ch)
                dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
            hasDryBuffer = true;
        }

        slot.instance->processBlock (buffer, midi);

        if (slot.dryWet < 0.999f && hasDryBuffer)
        {
            const float wetGain = slot.dryWet;
            const float dryGain = 1.0f - wetGain;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* wetPtr = buffer.getWritePointer (ch);
                const auto* dryPtr = dryBuffer.getReadPointer (ch);
                for (int s = 0; s < numSamples; ++s)
                    wetPtr[s] = (wetPtr[s] * wetGain) + (dryPtr[s] * dryGain);
            }

            for (int ch = 0; ch < numChannels; ++ch)
                dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        }
    }
}

juce::AudioPluginInstance* AUv3Manager::getSlotInstance (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < numSlots)
        return slots[slotIndex].instance.get();
    return nullptr;
}

juce::AudioProcessorEditor* AUv3Manager::createSlotEditor (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < numSlots && slots[slotIndex].instance != nullptr)
        return slots[slotIndex].instance->createEditorIfNeeded();
    return nullptr;
}

#endif
