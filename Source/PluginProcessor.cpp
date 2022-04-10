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
      apvts(*this, nullptr, "Parameters", createParameters()),
      lowShelfFilter(juce::dsp::IIR::Coefficients<float>::makeLowShelf(
          44100, 20.0f, 1.0f, 0.7f)),
      highShelfFilter(juce::dsp::IIR::Coefficients<float>::makeHighShelf(
          44100, 20000.0f, 1.0f, 0.7f))
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

  soundtouch.setSampleRate(sampleRate);
  soundtouch.setChannels(1);

  inputGainer.prepare(spec);
  inputGainer.reset();
  outputGainer.prepare(spec);
  outputGainer.reset();
  dryWetMixer.prepare(spec);
  dryWetMixer.reset();
  delay.prepare(spec);
  delay.setMaximumDelayInSamples(sampleRate);
  delay.reset();
  convolver.prepare(spec);
  convolver.reset();

  lowShelfFilter.prepare(spec);
  lowShelfFilter.reset();
  highShelfFilter.prepare(spec);
  highShelfFilter.reset();
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

  // acquire parameters from AudioProcessorValueTreeState
  auto inputGainValue = apvts.getRawParameterValue("InputGain");
  auto outputGainValue = apvts.getRawParameterValue("OutputGain");
  auto dryWetMixValue = apvts.getRawParameterValue("DryWetMix");
  auto preDelayTimeValue = apvts.getRawParameterValue("PreDelayTime");
  auto stereoWidthValue = apvts.getRawParameterValue("StereoWidth");
  auto isBypassed = apvts.getRawParameterValue("Bypassed");
  updateFilterParameters();

  if (isBypassed->load() == true) {
    return;
  }

  inputGainer.setGainDecibels(inputGainValue->load());
  outputGainer.setGainDecibels(outputGainValue->load());
  dryWetMixer.setWetMixProportion(dryWetMixValue->load() / 100.0f);
  delay.setDelay(preDelayTimeValue->load() / 1000.0 * this->getSampleRate());

  auto block = juce::dsp::AudioBlock<float>(buffer);
  auto context = juce::dsp::ProcessContextReplacing<float>(block);
  inputGainer.process(context);
  dryWetMixer.pushDrySamples(block);
  convolver.process(context);
  delay.process(context);

  // set stereo width using mid/side technique
  if (context.getInputBlock().getNumChannels() == 2) {
    const float width = stereoWidthValue->load() / 100.0;
    for (int sample = 0; sample < context.getInputBlock().getNumSamples();
         ++sample) {
      float left = context.getInputBlock().getSample(0, sample);
      float right = context.getInputBlock().getSample(1, sample);
      context.getOutputBlock().setSample(
          0, sample, left * (1 + width) / 2 + right * (1 - width) / 2);
      context.getOutputBlock().setSample(
          1, sample, left * (1 - width) / 2 + right * (1 + width) / 2);
    }
  }

  lowShelfFilter.process(context);
  highShelfFilter.process(context);

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

void ConekoAudioProcessor::setIRBufferSize(int newNumChannels,
                                           int newNumSamples,
                                           bool keepExistingContent,
                                           bool clearExtraSpace,
                                           bool avoidReallocating) {
  originalIRBuffer.setSize(newNumChannels, newNumSamples, keepExistingContent,
                           clearExtraSpace, avoidReallocating);
}

juce::AudioBuffer<float> &ConekoAudioProcessor::getOriginalIR() {
  return originalIRBuffer;
}

juce::AudioBuffer<float> &ConekoAudioProcessor::getModifiedIR() {
  return modifiedIRBuffer;
}

void ConekoAudioProcessor::loadImpulseResponse() {
  // normalized IR signal
  float globalMaxMagnitude =
      originalIRBuffer.getMagnitude(0, originalIRBuffer.getNumSamples());
  originalIRBuffer.applyGain(1.0f / (globalMaxMagnitude + 0.01));

  // trim IR signal
  int numSamples = originalIRBuffer.getNumSamples();
  int blockSize = static_cast<int>(std::floor(this->getSampleRate()) / 100);
  int startBlockNum = 0;
  int endBlockNum = numSamples / blockSize;
  float localMaxMagnitude = 0.0f;
  while ((startBlockNum + 1) * blockSize < numSamples) {
    localMaxMagnitude =
        originalIRBuffer.getMagnitude(startBlockNum * blockSize, blockSize);
    // find the start position of IR
    if (localMaxMagnitude > 0.001) {
      break;
    }
    ++startBlockNum;
  }
  localMaxMagnitude = 0.0f;
  while ((endBlockNum - 1) * blockSize > 0) {
    --endBlockNum;
    localMaxMagnitude =
        originalIRBuffer.getMagnitude(endBlockNum * blockSize, blockSize);
    // find the time to decay by 60 dB (T60)
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
  modifiedIRBuffer.setSize(originalIRBuffer.getNumChannels(), trimmedNumSamples,
                           false, true, false);
  for (int channel = 0; channel < originalIRBuffer.getNumChannels();
       ++channel) {
    for (int sample = 0; sample < trimmedNumSamples; ++sample) {
      modifiedIRBuffer.setSample(
          channel, sample,
          originalIRBuffer.getSample(channel,
                                     sample + startBlockNum * blockSize));
    }
  }

  originalIRBuffer.makeCopyOf(modifiedIRBuffer);

  auto decayTimeParam = apvts.getParameter("DecayTime");
  double decayTime =
      static_cast<double>(trimmedNumSamples) / this->getSampleRate();
  decayTimeParam->beginChangeGesture();
  decayTimeParam->setValueNotifyingHost(
      decayTimeParam->convertTo0to1(decayTime));
  decayTimeParam->endChangeGesture();

  updateImpulseResponse(modifiedIRBuffer);
}

void ConekoAudioProcessor::updateImpulseResponse(
    juce::AudioBuffer<float> irBuffer) {
  convolver.loadImpulseResponse(std::move(irBuffer), this->getSampleRate(),
                                juce::dsp::Convolution::Stereo::yes,
                                juce::dsp::Convolution::Trim::no,
                                juce::dsp::Convolution::Normalise::yes);
}

void ConekoAudioProcessor::updateIRParameters() {
  if (originalIRBuffer.getNumSamples() < 1) {
    return;
  }

  // stretch IR according to decay time
  auto decayTimeValue = apvts.getRawParameterValue("DecayTime");
  int decaySample = static_cast<int>(
      std::round(decayTimeValue->load() * this->getSampleRate()));
  double stretchRatio =
      originalIRBuffer.getNumSamples() / static_cast<double>(decaySample);

  int numChannels = originalIRBuffer.getNumChannels();
  soundtouch.setTempo(stretchRatio);
  modifiedIRBuffer.setSize(numChannels, decaySample, false, true, false);
  for (int channel = 0; channel < numChannels; ++channel) {
    soundtouch.putSamples(originalIRBuffer.getReadPointer(channel),
                          originalIRBuffer.getNumSamples());
    soundtouch.receiveSamples(modifiedIRBuffer.getWritePointer(channel),
                              decaySample);
    soundtouch.clear();
  }

  // delay IR according to pre-delay time
  // auto preDelayTimeValue = apvts.getRawParameterValue("PreDelayTime");
  // int preDelaySample = static_cast<int>(std::round(preDelayTimeValue->load())
  // /
  //                                      1000 * this->getSampleRate());
  // juce::AudioBuffer<float> tempBuffer(modifiedIRBuffer);
  // modifiedIRBuffer.setSize(numChannels,
  //                         preDelaySample + tempBuffer.getNumSamples(), false,
  //                         false, false);
  // modifiedIRBuffer.clear();
  // for (int channel = 0; channel < numChannels; ++channel) {
  //  modifiedIRBuffer.copyFrom(channel, preDelaySample,
  //                            tempBuffer.getReadPointer(channel),
  //                            tempBuffer.getNumSamples());
  //}

  // reverse of the IR
  auto isReversed = apvts.getRawParameterValue("Reversed");
  if (isReversed->load() == true) {
    modifiedIRBuffer.reverse(0, modifiedIRBuffer.getNumSamples());
  }

  updateImpulseResponse(modifiedIRBuffer);
}

void ConekoAudioProcessor::updateFilterParameters() {
  const float sampleRate = this->getSampleRate();

  auto lowShelfFreqValue = apvts.getRawParameterValue("LowShelfFreq");
  auto lowShelfGainValue = apvts.getRawParameterValue("LowShelfGain");
  auto highShelfFreqValue = apvts.getRawParameterValue("HighShelfFreq");
  auto highShelfGainValue = apvts.getRawParameterValue("HighShelfGain");
  *lowShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
      sampleRate, lowShelfFreqValue->load(), 0.7f,
      juce::Decibels::decibelsToGain(lowShelfGainValue->load()));
  *highShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
      sampleRate, highShelfFreqValue->load(), 0.7f,
      juce::Decibels::decibelsToGain(highShelfGainValue->load()));
}

juce::AudioProcessorValueTreeState::ParameterLayout
ConekoAudioProcessor::createParameters() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

  juce::NormalisableRange<float> gainRange =
      juce::NormalisableRange<float>(-72.0f, 36.0f, 0.1f);
  gainRange.setSkewForCentre(0.0f);
  juce::NormalisableRange<float> dryWetMixRange =
      juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f);
  juce::NormalisableRange<float> decayTimeRange =
      juce::NormalisableRange<float>(0.10f, 8.00f, 0.01f);
  decayTimeRange.setSkewForCentre(3.0f);
  juce::NormalisableRange<float> preDelayTimeRange =
      juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.5f);
  juce::NormalisableRange<float> stereoWidthRange =
      juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f);
  juce::NormalisableRange<float> lowShelfCutoffFreqRange =
      juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f);
  lowShelfCutoffFreqRange.setSkewForCentre(400.0f);
  juce::NormalisableRange<float> lowShelfGainRange =
      juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f);
  lowShelfGainRange.setSkewForCentre(0.0f);
  juce::NormalisableRange<float> highShelfCutoffFreqRange =
      juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f);
  highShelfCutoffFreqRange.setSkewForCentre(4000.0f);
  juce::NormalisableRange<float> highShelfGainRange =
      juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f);

  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "Reversed", "Reversed", false));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "Bypassed", "Bypassed", false));
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
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "StereoWidth", "Width", stereoWidthRange, 100.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "LowShelfFreq", "LowFreq", lowShelfCutoffFreqRange, 20.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "LowShelfGain", "LowGain", lowShelfGainRange, 0.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "HighShelfFreq", "HighFreq", highShelfCutoffFreqRange, 20000.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "HighShelfGain", "HighGain", highShelfGainRange, 0.0f));
  return {parameters.begin(), parameters.end()};
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new ConekoAudioProcessor();
}
