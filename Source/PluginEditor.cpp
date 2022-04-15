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
  juce::LookAndFeel::setDefaultLookAndFeel(&customStyle);

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

  createSlider(inputGainSlider, " dB");
  createLabel(inputGainLabel, "Input", &inputGainSlider);
  inputGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "InputGain", inputGainSlider);

  createSlider(outputGainSlider, " dB");
  createLabel(outputGainLabel, "Output", &outputGainSlider);
  outputGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "OutputGain", outputGainSlider);

  createSlider(dryWetMixSlider, " %");
  createLabel(dryWetMixLabel, "Mix", &dryWetMixSlider);
  dryWetMixSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "DryWetMix", dryWetMixSlider);

  createSlider(decayTimeSlider, " s");
  decayTimeSlider.setEnabled(enableIRParameters);
  decayTimeSlider.onDragEnd = [this] {
    audioProcessor.updateIRParameters();
    shouldPaintWaveform = true;
    repaint();
  };
  createLabel(decayTimeLabel, "Decay", &decayTimeSlider);
  decayTimeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "DecayTime", decayTimeSlider);

  createSlider(preDelayTimeSlider, " ms");
  // preDelayTimeSlider.onDragEnd = [this] {
  //  audioProcessor.updateIRParameters();
  //  shouldPaintWaveform = true;
  //  repaint();
  //};
  createLabel(preDelayTimeLabel, "Pre-delay", &preDelayTimeSlider);
  preDelayTimeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "PreDelayTime", preDelayTimeSlider);

  createSlider(stereoWidthSlider, " %");
  createLabel(stereoWidthLabel, "Width", &stereoWidthSlider);
  stereoWidthSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "StereoWidth", stereoWidthSlider);

  createSlider(lowShelfFreqSlider, " Hz");
  createLabel(lowShelfFreqLabel, "LowFreq", &lowShelfFreqSlider);
  lowShelfFreqSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "LowShelfFreq", lowShelfFreqSlider);

  createSlider(lowShelfGainSlider, " dB");
  createLabel(lowShelfGainLabel, "LowGain", &lowShelfGainSlider);
  lowShelfGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "LowShelfGain", lowShelfGainSlider);

  createSlider(highShelfFreqSlider, " Hz");
  createLabel(highShelfFreqLabel, "HighFreq", &highShelfFreqSlider);
  highShelfFreqSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "HighShelfFreq", highShelfFreqSlider);

  createSlider(highShelfGainSlider, " dB");
  createLabel(highShelfGainLabel, "HighGain", &highShelfGainSlider);
  highShelfGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "HighShelfGain", highShelfGainSlider);
}

ConekoAudioProcessorEditor::~ConekoAudioProcessorEditor() {
  juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

//==============================================================================
void ConekoAudioProcessorEditor::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)

  // g.fillAll(
  //     getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
  g.fillAll(juce::Colour::fromRGB(252, 248, 237));
  g.setFont(32.0f);
  g.setColour(juce::Colour::fromRGB(111, 76, 91));
  g.drawFittedText("Coneko", getWidth() - 80 - 15, 15, 80, 20,
                   juce::Justification::centred, 1);

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

void ConekoAudioProcessorEditor::createSlider(juce::Slider &slider,
                                              juce::String textValueSuffix) {
  addAndMakeVisible(slider);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextValueSuffix(textValueSuffix);
  slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow,
                         false, 60, 15);
  // slider.setPopupDisplayEnabled(true, true, this);
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
