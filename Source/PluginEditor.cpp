/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
ConekoAudioProcessorEditor::ConekoAudioProcessorEditor(ConekoAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(750, 300);

  // set AudioFormatManager for reading IR file
  formatManager.registerBasicFormats();

  const auto sliderStyle = juce::Slider::RotaryHorizontalVerticalDrag;
  const auto sliderLabelJustification = juce::Justification::centred;

  addAndMakeVisible(openIRFileButton);
  openIRFileButton.setButtonText("Open IR File...");
  openIRFileButton.onClick = [this] { openButtonClicked(); };
  addAndMakeVisible(irFileLabel);
  irFileLabel.setText("", juce::dontSendNotification);
  irFileLabel.setJustificationType(juce::Justification::centredLeft);

  addAndMakeVisible(reverseButton);
  reverseButton.setButtonText("Reverse IR");
  reverseButton.setEnabled(enableIRParameters);
  reverseButton.onClick = [this] {
    audioProcessor.updateIRParameters();
    shouldPaintWaveform = true;
    repaint();
  };
  reverseButtonAttachment = std::make_unique<APVTS::ButtonAttachment>(
      audioProcessor.apvts, "Reversed", reverseButton);

  addAndMakeVisible(bypassButton);
  bypassButton.setButtonText("Bypass");
  bypassButtonAttachment = std::make_unique<APVTS::ButtonAttachment>(
      audioProcessor.apvts, "Bypassed", bypassButton);

  addAndMakeVisible(inputGainSlider);
  inputGainSlider.setSliderStyle(sliderStyle);
  inputGainSlider.setTextValueSuffix(" dB");
  inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  // inputLevelSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(inputGainLabel, "Input", &inputGainSlider);
  inputGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "InputGain", inputGainSlider);

  addAndMakeVisible(outputGainSlider);
  outputGainSlider.setSliderStyle(sliderStyle);
  outputGainSlider.setTextValueSuffix(" dB");
  outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  // outputLevelSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(outputGainLabel, "Output", &outputGainSlider);
  outputGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "OutputGain", outputGainSlider);

  addAndMakeVisible(dryWetMixSlider);
  dryWetMixSlider.setSliderStyle(sliderStyle);
  dryWetMixSlider.setTextValueSuffix(" %");
  dryWetMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  // wetMixSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(dryWetMixLabel, "Mix", &dryWetMixSlider);
  dryWetMixSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "DryWetMix", dryWetMixSlider);

  addAndMakeVisible(decayTimeSlider);
  decayTimeSlider.setSliderStyle(sliderStyle);
  decayTimeSlider.setTextValueSuffix(" s");
  decayTimeSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // decayTimeSlider.setPopupDisplayEnabled(true, true, this);
  decayTimeSlider.setEnabled(enableIRParameters);
  decayTimeSlider.onDragEnd = [this] {
    audioProcessor.updateIRParameters();
    shouldPaintWaveform = true;
    repaint();
  };
  createLabel(decayTimeLabel, "Decay", &decayTimeSlider);
  decayTimeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "DecayTime", decayTimeSlider);

  addAndMakeVisible(preDelayTimeSlider);
  preDelayTimeSlider.setSliderStyle(sliderStyle);
  preDelayTimeSlider.setTextValueSuffix(" ms");
  preDelayTimeSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // preDelayTimeSlider.setPopupDisplayEnabled(true, true, this);
  // preDelayTimeSlider.onDragEnd = [this] {
  //  audioProcessor.updateIRParameters();
  //  shouldPaintWaveform = true;
  //  repaint();
  //};
  createLabel(preDelayTimeLabel, "Pre-delay", &preDelayTimeSlider);
  preDelayTimeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "PreDelayTime", preDelayTimeSlider);

  addAndMakeVisible(stereoWidthSlider);
  stereoWidthSlider.setSliderStyle(sliderStyle);
  stereoWidthSlider.setTextValueSuffix(" %");
  stereoWidthSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // stereoWidthSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(stereoWidthLabel, "Width", &stereoWidthSlider);
  stereoWidthSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "StereoWidth", stereoWidthSlider);

  addAndMakeVisible(lowShelfFreqSlider);
  lowShelfFreqSlider.setSliderStyle(sliderStyle);
  lowShelfFreqSlider.setTextValueSuffix(" Hz");
  lowShelfFreqSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // lowShelfFreqSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(lowShelfFreqLabel, "LowFreq", &lowShelfFreqSlider);
  lowShelfFreqSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "LowShelfFreq", lowShelfFreqSlider);

  addAndMakeVisible(lowShelfGainSlider);
  lowShelfGainSlider.setSliderStyle(sliderStyle);
  lowShelfGainSlider.setTextValueSuffix(" dB");
  lowShelfGainSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // lowShelfFreqSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(lowShelfGainLabel, "LowGain", &lowShelfGainSlider);
  lowShelfGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "LowShelfGain", lowShelfGainSlider);

  addAndMakeVisible(highShelfFreqSlider);
  highShelfFreqSlider.setSliderStyle(sliderStyle);
  highShelfFreqSlider.setTextValueSuffix(" Hz");
  highShelfFreqSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // highShelfFreqSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(highShelfFreqLabel, "HighFreq", &highShelfFreqSlider);
  highShelfFreqSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "HighShelfFreq", highShelfFreqSlider);

  addAndMakeVisible(highShelfGainSlider);
  highShelfGainSlider.setSliderStyle(sliderStyle);
  highShelfGainSlider.setTextValueSuffix(" dB");
  highShelfGainSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // lowShelfFreqSlider.setPopupDisplayEnabled(true, true, this);
  createLabel(highShelfGainLabel, "HighGain", &highShelfGainSlider);
  highShelfGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "HighShelfGain", highShelfGainSlider);
}

ConekoAudioProcessorEditor::~ConekoAudioProcessorEditor() {}

//==============================================================================
void ConekoAudioProcessorEditor::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)

  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
  // g.fillAll(juce::Colours::white);
  // g.setColour(juce::Colours::black);
  g.setFont(15.0f);
  g.drawFittedText("Coneko", 0, 0, getWidth() - 10, 30,
                   juce::Justification::centredRight, 1);

  g.setColour(juce::Colour::fromRGB(158, 119, 119));

  if (shouldPaintWaveform == true) {
    const int waveformWidth = 80 * 3;
    const int waveformHeight = 100;

    juce::Path waveformPath;
    waveformValues.clear();
    waveformPath.startNewSubPath(15, waveformHeight + 60);

    auto buffer = audioProcessor.getModifiedIR();
    if (buffer.getNumSamples() < 1) {
      buffer = audioProcessor.getOriginalIR();
    }
    const float waveformResolution = 1024.0f;
    const int ratio =
        static_cast<int>(buffer.getNumSamples() / waveformResolution);

    auto bufferPointer = buffer.getReadPointer(0);
    for (int sample = 0; sample < buffer.getNumSamples(); sample += ratio) {
      waveformValues.push_back(juce::Decibels::gainToDecibels<float>(
          std::fabsf(bufferPointer[sample]), -72.0f));
    }
    for (int xPos = 0; xPos < waveformValues.size(); ++xPos) {
      auto yPos = juce::jmap<float>(waveformValues[xPos], -72.0f, 0.0f,
                                    waveformHeight + 60, 60);
      waveformPath.lineTo(15 + xPos / waveformResolution * waveformWidth, yPos);
    }

    g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

    shouldPaintWaveform = false;
  }
}

void ConekoAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
  const int topBottomMargin = 15;
  const int leftRightMargin = 15;

  const int dialWidth = 80;
  const int dialHeight = 90;

  openIRFileButton.setBounds(leftRightMargin, topBottomMargin, dialWidth * 3,
                             40);
  irFileLabel.setBounds(leftRightMargin, topBottomMargin + 45, dialWidth * 3,
                        20);
  reverseButton.setBounds(leftRightMargin + dialWidth * 2, topBottomMargin + 40,
                          dialWidth, 30);
  bypassButton.setBounds(getWidth() - leftRightMargin - dialWidth * 3,
                         topBottomMargin, dialWidth, 20);
  inputGainSlider.setBounds(leftRightMargin,
                            getHeight() - topBottomMargin - dialHeight,
                            dialWidth, dialHeight);
  outputGainSlider.setBounds(leftRightMargin + dialWidth,
                             getHeight() - topBottomMargin - dialHeight,
                             dialWidth, dialHeight);
  dryWetMixSlider.setBounds(leftRightMargin + dialWidth * 2,
                            getHeight() - topBottomMargin - dialHeight,
                            dialWidth, dialHeight);
  decayTimeSlider.setBounds(leftRightMargin + dialWidth * 3,
                            getHeight() - topBottomMargin - dialHeight * 3 + 30,
                            dialWidth * 3, dialHeight * 3 - 30);
  preDelayTimeSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 3,
                               topBottomMargin + dialHeight / 3 * 2, dialWidth,
                               dialHeight);
  stereoWidthSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 3,
                              getHeight() - topBottomMargin - dialHeight,
                              dialWidth, dialHeight);
  lowShelfFreqSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 2,
                               topBottomMargin + dialHeight / 3 * 2, dialWidth,
                               dialHeight);
  lowShelfGainSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 2,
                               getHeight() - topBottomMargin - dialHeight,
                               dialWidth, dialHeight);
  highShelfFreqSlider.setBounds(getWidth() - leftRightMargin - dialWidth,
                                topBottomMargin + dialHeight / 3 * 2, dialWidth,
                                dialHeight);
  highShelfGainSlider.setBounds(getWidth() - leftRightMargin - dialWidth,
                                getHeight() - topBottomMargin - dialHeight,
                                dialWidth, dialHeight);
}

void ConekoAudioProcessorEditor::openButtonClicked() {
  fileChooser = std::make_unique<juce::FileChooser>(
      "Choose a support IR File (WAV, AIFF, OGG)...", juce::File(),
      "*.wav;*.aif;*.aiff;*.ogg", true, false);
  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;
  fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file != juce::File()) {
      // update text of IR file label
      irFileLabel.setText(file.getFileName(), juce::dontSendNotification);
      irFileLabel.repaint();

      auto *reader = formatManager.createReaderFor(file);
      if (reader != nullptr) {
        audioProcessor.setIRBufferSize(
            static_cast<int>(reader->numChannels),
            static_cast<int>(reader->lengthInSamples));
        reader->read(&audioProcessor.getOriginalIR(), 0,
                     static_cast<int>(reader->lengthInSamples), 0, true, true);
        audioProcessor.loadImpulseResponse();

        shouldPaintWaveform = true;
        enableIRParameters = true;
        reverseButton.setEnabled(enableIRParameters);
        decayTimeSlider.setEnabled(enableIRParameters);
        repaint();
      }
    }
  });
}

void ConekoAudioProcessorEditor::createLabel(juce::Label &label,
                                             juce::String text,
                                             juce::Component *slider) {
  addAndMakeVisible(label);
  label.setText(text, juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centred);
  label.setBorderSize(juce::BorderSize<int>(0));
  label.attachToComponent(slider, false);
}
