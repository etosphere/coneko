/*
 ==============================================================================

   This file contains the basic framework code for a JUCE plugin processor.

 ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ConekoAudioProcessor::ConekoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
}

ConekoAudioProcessor::~ConekoAudioProcessor() {}

//==============================================================================
const juce::String ConekoAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool ConekoAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool ConekoAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool ConekoAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double ConekoAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ConekoAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int ConekoAudioProcessor::getCurrentProgram() { return 0; }

void ConekoAudioProcessor::setCurrentProgram(int index) {}

const juce::String ConekoAudioProcessor::getProgramName(int index) {
  return {};
}

void ConekoAudioProcessor::changeProgramName(int index,
                                             const juce::String &newName) {}

//==============================================================================
void ConekoAudioProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {
  // pre-initialization process
  juce::dsp::ProcessSpec spec = juce::dsp::ProcessSpec();
  spec.sampleRate = sampleRate;
  spec.numChannels = getTotalNumOutputChannels();
  spec.maximumBlockSize = samplesPerBlock;

  inputGainer.prepare(spec);
  inputGainer.reset();
  outputGainer.prepare(spec);
  outputGainer.reset();
  dryWetMixer.prepare(spec);
  dryWetMixer.reset();
  convolver.prepare(spec);
  convolver.reset();
}

void ConekoAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ConekoAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void ConekoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                        juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  auto inputGainValue = apvts.getRawParameterValue("InputGain");
  auto outputGainValue = apvts.getRawParameterValue("OutputGain");
  auto dryWetMixValue = apvts.getRawParameterValue("DryWetMix");
  // for (int channel = 0; channel < totalNumInputChannels; ++channel) {
  //   auto *channelData = buffer.getWritePointer(channel);
  //   for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {
  //      channelData[sample] *= 1;
  //   }
  // }

  inputGainer.setGainDecibels(inputGainValue->load());
  outputGainer.setGainDecibels(outputGainValue->load());
  dryWetMixer.setWetMixProportion(dryWetMixValue->load() / 100.0f);
  auto block = juce::dsp::AudioBlock<float>(buffer);
  auto context = juce::dsp::ProcessContextReplacing<float>(block);
  inputGainer.process(context);
  dryWetMixer.pushDrySamples(block);
  convolver.process(context);
  dryWetMixer.mixWetSamples(block);
  outputGainer.process(context);
}

//==============================================================================
bool ConekoAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *ConekoAudioProcessor::createEditor() {
  return new ConekoAudioProcessorEditor(*this);
}

//==============================================================================
void ConekoAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
}

void ConekoAudioProcessor::setStateInformation(const void *data,
                                               int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
}

void ConekoAudioProcessor::loadImpulseResponse() {
  // normalized IR signal
  float globalMaxMagnitude =
      rawIRBuffer.getMagnitude(0, rawIRBuffer.getNumSamples());
  rawIRBuffer.applyGain(1.0f / (globalMaxMagnitude + 0.01));

  // trim IR signal
  int numSamples = rawIRBuffer.getNumSamples();
  int blockSize = static_cast<int>(std::floor(this->getSampleRate()) / 100);
  int startBlockNum = 0;
  int endBlockNum = numSamples / blockSize;
  float localMaxMagnitude = 0.0f;
  while ((startBlockNum + 1) * blockSize < numSamples) {
    localMaxMagnitude =
        rawIRBuffer.getMagnitude(startBlockNum * blockSize, blockSize);
    if (localMaxMagnitude > 0.001) {
      break;
    }
    ++startBlockNum;
  }
  localMaxMagnitude = 0.0f;
  while ((endBlockNum - 1) * blockSize > 0) {
    --endBlockNum;
    localMaxMagnitude =
        rawIRBuffer.getMagnitude(endBlockNum * blockSize, blockSize);
    if (localMaxMagnitude > 0.001) {
      break;
    }
  }

  int trimmedNumSamples;
  if (endBlockNum * blockSize < numSamples) {
    trimmedNumSamples = (endBlockNum - startBlockNum) * blockSize - 1;
  } else {
    trimmedNumSamples = numSamples - startBlockNum * blockSize;
  }
  modifiedIRBuffer.setSize(rawIRBuffer.getNumChannels(), trimmedNumSamples,
                           false, true, false);
  for (int channel = 0; channel < rawIRBuffer.getNumChannels(); ++channel) {
    for (int sample = 0; sample < trimmedNumSamples; ++sample) {
      modifiedIRBuffer.setSample(
          channel, sample,
          rawIRBuffer.getSample(channel, sample + startBlockNum * blockSize));
    }
  }

  auto decayTimeParam = apvts.getParameter("DecayTime");
  double decayTime =
      static_cast<double>(trimmedNumSamples) / this->getSampleRate();
  decayTimeParam->setValueNotifyingHost(decayTime / (8.0 - 0.01));

  updateImpulseResponse();
}

void ConekoAudioProcessor::updateImpulseResponse() {
  convolver.loadImpulseResponse(
      std::move(modifiedIRBuffer), this->getSampleRate(),
      juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no,
      juce::dsp::Convolution::Normalise::yes);
}

juce::AudioProcessorValueTreeState::ParameterLayout
ConekoAudioProcessor::createParameters() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

  juce::NormalisableRange<float> gainRange =
      juce::NormalisableRange<float>(-72.0f, 36.0f, 0.1f);
  gainRange.setSkewForCentre(0.0f);
  juce::NormalisableRange<float> dryWetMixRange =
      juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f);
  dryWetMixRange.setSkewForCentre(50.0f);
  juce::NormalisableRange<float> decayTimeRange =
      juce::NormalisableRange<float>(0.01f, 8.00f, 0.01f);
  decayTimeRange.setSkewForCentre(3.0f);
  juce::NormalisableRange<float> preDelayTimeRange =
      juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.5f);

  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "InputGain", "Input Gain", gainRange, 0.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "OutputGain", "Output Gain", gainRange, 0.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DryWetMix", "Mix", dryWetMixRange, 100.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DecayTime", "Decay", decayTimeRange, 3.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "PreDelayTime", "Pre-delay", preDelayTimeRange, 0.0f));
  return {parameters.begin(), parameters.end()};
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new ConekoAudioProcessor();
}
