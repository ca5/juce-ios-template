#include "AUv3Manager.h"
#include <cmath>

#if JUCE_IOS || JUCE_MAC
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#endif

AUv3Manager::AUv3Manager()
{
    juce::addDefaultFormatsToManager (formatManager);

    for (int i = 0; i < numSlots; ++i)
    {
        slots[i].slotId = i + 1;
        slots[i].enabled = true;
        slots[i].dryWet = 1.0f;
    }

   #if JUCE_IOS
    @autoreleasepool
    {
        notificationObserver = (void*) [[NSNotificationCenter defaultCenter]
            addObserverForName:AVAudioUnitComponentManagerRegistrationsChangedNotification
                        object:[AVAudioUnitComponentManager sharedAudioUnitComponentManager]
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification* /*note*/) {
                        NSLog(@"[AUv3Manager] AVAudioUnitComponentManagerRegistrationsChangedNotification received!");
                        scanInstalledPlugins();
                        if (onPluginsChanged)
                            onPluginsChanged();
                    }];
    }
   #endif

    scanInstalledPlugins();
}

AUv3Manager::~AUv3Manager()
{
   #if JUCE_IOS
    if (notificationObserver != nullptr)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:(id)notificationObserver];
        notificationObserver = nullptr;
    }
   #endif
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

   #if JUCE_IOS
    @autoreleasepool
    {
        // 1. Enumerate via AVAudioUnitComponentManager (AUv3 AppExtensions)
        AVAudioUnitComponentManager* compMgr = [AVAudioUnitComponentManager sharedAudioUnitComponentManager];
        NSArray<AVAudioUnitComponent*>* components = [compMgr componentsPassingTest:^BOOL (AVAudioUnitComponent* comp, BOOL* /*stop*/) {
            const AudioComponentDescription desc = comp.audioComponentDescription;
            return (desc.componentType == kAudioUnitType_Effect
                 || desc.componentType == kAudioUnitType_MusicEffect
                 || desc.componentType == kAudioUnitType_MusicDevice
                 || desc.componentType == kAudioUnitType_MIDIProcessor);
        }];

        for (AVAudioUnitComponent* comp in components)
        {
            const AudioComponentDescription desc = comp.audioComponentDescription;
            const OSType type = desc.componentType;
            const bool isEffect = (type == kAudioUnitType_MusicEffect || type == kAudioUnitType_Effect);
            const bool isInstrument = (type == kAudioUnitType_MusicDevice);

            juce::PluginDescription pd;
            pd.name = juce::String::fromCFString ((CFStringRef) comp.name);
            pd.descriptiveName = pd.name;
            pd.manufacturerName = juce::String::fromCFString ((CFStringRef) comp.manufacturerName);
            pd.category = juce::String::fromCFString ((CFStringRef) comp.localizedTypeName);
            if (pd.category.isEmpty())
                pd.category = isInstrument ? "Instrument" : "Effect";

            // Format identifier: "AudioUnit:Effects/type,subtype,mfg"
            juce::String ident ("AudioUnit:");
            if (isEffect)
                ident << "Effects/";
            else if (type == kAudioUnitType_MIDIProcessor)
                ident << "MidiEffects/";
            else if (isInstrument)
                ident << "Synths/";
            else
                ident << "Effects/";

            auto osTypeToString = [] (OSType t) -> juce::String
            {
                char str[5] = { 0 };
                str[0] = (char) ((t >> 24) & 0xff);
                str[1] = (char) ((t >> 16) & 0xff);
                str[2] = (char) ((t >> 8) & 0xff);
                str[3] = (char) (t & 0xff);
                return juce::String (str).trim();
            };

            ident << osTypeToString (desc.componentType) << ","
                  << osTypeToString (desc.componentSubType) << ","
                  << osTypeToString (desc.componentManufacturer);

            pd.fileOrIdentifier = ident;
            pd.uniqueId = pd.deprecatedUid = ((int) desc.componentType)
                                           ^ ((int) desc.componentSubType)
                                           ^ ((int) desc.componentManufacturer);
            pd.pluginFormatName = "AudioUnit";
            pd.isInstrument = isInstrument;
            pd.numInputChannels = 2;
            pd.numOutputChannels = 2;

            knownPluginList.addType (pd);
        }

        // 2. Query AudioComponentFindNext for built-in Apple AudioUnits
        AudioComponent comp = nullptr;
        for (;;)
        {
            AudioComponentDescription desc;
            memset (&desc, 0, sizeof (desc));
            comp = AudioComponentFindNext (comp, &desc);
            if (comp == nullptr) break;

            if (desc.componentType != kAudioUnitType_Effect
             && desc.componentType != kAudioUnitType_MusicEffect
             && desc.componentType != kAudioUnitType_MusicDevice
             && desc.componentType != kAudioUnitType_MIDIProcessor)
                continue;

            CFStringRef cfName = nullptr;
            if (AudioComponentCopyName (comp, &cfName) == noErr && cfName != nullptr)
            {
                juce::String fullName = juce::String::fromCFString (cfName);
                CFRelease (cfName);

                juce::String mfgName = "Apple";
                juce::String plugName = fullName;
                if (fullName.containsChar (':'))
                {
                    mfgName = fullName.upToFirstOccurrenceOf (":", false, false).trim();
                    plugName = fullName.fromFirstOccurrenceOf (":", false, false).trim();
                }

                auto osTypeToString = [] (OSType t) -> juce::String
                {
                    char str[5] = { 0 };
                    str[0] = (char) ((t >> 24) & 0xff);
                    str[1] = (char) ((t >> 16) & 0xff);
                    str[2] = (char) ((t >> 8) & 0xff);
                    str[3] = (char) (t & 0xff);
                    return juce::String (str).trim();
                };

                const bool isEffect = (desc.componentType == kAudioUnitType_MusicEffect || desc.componentType == kAudioUnitType_Effect);
                juce::String ident ("AudioUnit:");
                if (isEffect)
                    ident << "Effects/";
                else if (desc.componentType == kAudioUnitType_MIDIProcessor)
                    ident << "MidiEffects/";
                else if (desc.componentType == kAudioUnitType_MusicDevice)
                    ident << "Synths/";
                else
                    ident << "Effects/";

                ident << osTypeToString (desc.componentType) << ","
                      << osTypeToString (desc.componentSubType) << ","
                      << osTypeToString (desc.componentManufacturer);

                bool alreadyExists = false;
                for (const auto& existing : knownPluginList.getTypes())
                {
                    if (existing.fileOrIdentifier == ident)
                    {
                        alreadyExists = true;
                        break;
                    }
                }

                if (!alreadyExists)
                {
                    juce::PluginDescription pd;
                    pd.name = plugName;
                    pd.descriptiveName = fullName;
                    pd.manufacturerName = mfgName;
                    pd.category = "Effect";
                    pd.fileOrIdentifier = ident;
                    pd.uniqueId = pd.deprecatedUid = ((int) desc.componentType)
                                                   ^ ((int) desc.componentSubType)
                                                   ^ ((int) desc.componentManufacturer);
                    pd.pluginFormatName = "AudioUnit";
                    pd.isInstrument = (desc.componentType == kAudioUnitType_MusicDevice);
                    pd.numInputChannels = 2;
                    pd.numOutputChannels = 2;
                    knownPluginList.addType (pd);
                }
            }
        }
    }
   #else
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
   #endif
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
                // Explicitly configure stereo bus layouts prior to prepareToPlay
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

    // Destroy instance outside spinlock to avoid any deadlocks with JUCE thread messaging
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

        // Process through active plugin
        slot.instance->processBlock (buffer, midi);

        // Apply Dry/Wet crossfade if wet is less than 100%
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
