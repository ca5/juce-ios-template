#include "PluginProcessor.h"
#include "PluginEditor.h"

JuceAppTemplateAudioProcessor::JuceAppTemplateAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                      ),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

JuceAppTemplateAudioProcessor::~JuceAppTemplateAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout JuceAppTemplateAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.8f
    ));

    return { params.begin(), params.end() };
}

const juce::String JuceAppTemplateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JuceAppTemplateAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JuceAppTemplateAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JuceAppTemplateAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JuceAppTemplateAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JuceAppTemplateAudioProcessor::getNumPrograms()
{
    return 1;
}

int JuceAppTemplateAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JuceAppTemplateAudioProcessor::setCurrentProgram (int)
{
}

const juce::String JuceAppTemplateAudioProcessor::getProgramName (int)
{
    return {};
}

void JuceAppTemplateAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void JuceAppTemplateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auv3Manager.prepareToPlay (sampleRate, samplesPerBlock);
}

void JuceAppTemplateAudioProcessor::releaseResources()
{
    auv3Manager.releaseResources();
}

bool JuceAppTemplateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
   #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
   #endif
}

void JuceAppTemplateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto gain = apvts.getRawParameterValue ("gain")->load();
    buffer.applyGain (gain);

    // AUv3 effect chain processing
    auv3Manager.processBlock (buffer, midiMessages);
}

bool JuceAppTemplateAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* JuceAppTemplateAudioProcessor::createEditor()
{
    return new JuceAppTemplateAudioProcessorEditor (*this);
}

void JuceAppTemplateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void JuceAppTemplateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JuceAppTemplateAudioProcessor();
}
