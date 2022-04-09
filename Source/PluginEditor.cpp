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

  addAndMakeVisible(openIRFileButton);
  openIRFileButton.setButtonText("Open IR File...");
  openIRFileButton.onClick = [this] { openButtonClicked(); };
  addAndMakeVisible(irFileLabel);
  irFileLabel.setText("", juce::dontSendNotification);
  irFileLabel.setJustificationType(juce::Justification::centredLeft);

  addAndMakeVisible(inputGainSlider);
  inputGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  inputGainSlider.setTextValueSuffix(" dB");
  inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  // inputLevelSlider.setPopupDisplayEnabled(true, true, this);
  // inputGainSlider.onValueChange = [this] {};
  addAndMakeVisible(inputGainLabel);
  inputGainLabel.setText("Input", juce::dontSendNotification);
  inputGainLabel.setJustificationType(juce::Justification::centred);
  inputGainLabel.attachToComponent(&inputGainSlider, false);
  inputGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "InputGain", inputGainSlider);

  addAndMakeVisible(outputGainSlider);
  outputGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  outputGainSlider.setTextValueSuffix(" dB");
  outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  // outputLevelSlider.setPopupDisplayEnabled(true, true, this);
  // outputGainSlider.onValueChange = [this] {};
  addAndMakeVisible(outputGainLabel);
  outputGainLabel.setText("Output", juce::dontSendNotification);
  outputGainLabel.setJustificationType(juce::Justification::centred);
  outputGainLabel.attachToComponent(&outputGainSlider, false);
  outputGainSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "OutputGain", outputGainSlider);

  addAndMakeVisible(dryWetMixSlider);
  dryWetMixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  dryWetMixSlider.setTextValueSuffix(" %");
  dryWetMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
  // wetMixSlider.setPopupDisplayEnabled(true, true, this);
  // dryWetMixSlider.onValueChange = [this] {};
  addAndMakeVisible(dryWetMixLabel);
  dryWetMixLabel.setText("Mix", juce::dontSendNotification);
  dryWetMixLabel.setJustificationType(juce::Justification::centred);
  dryWetMixLabel.attachToComponent(&dryWetMixSlider, false);
  dryWetMixSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "DryWetMix", dryWetMixSlider);

  addAndMakeVisible(decayTimeSlider);
  decayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  decayTimeSlider.setTextValueSuffix(" s");
  decayTimeSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // decayTimeSlider.setPopupDisplayEnabled(true, true, this);
  // decayTimeSlider.onValueChange = [this] {};
  addAndMakeVisible(decayTimeLabel);
  decayTimeLabel.setText("Decay", juce::dontSendNotification);
  decayTimeLabel.setJustificationType(juce::Justification::centred);
  decayTimeLabel.attachToComponent(&decayTimeSlider, false);
  decayTimeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "DecayTime", decayTimeSlider);

  addAndMakeVisible(preDelayTimeSlider);
  preDelayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  preDelayTimeSlider.setTextValueSuffix(" ms");
  preDelayTimeSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // preDelayTimeSlider.setPopupDisplayEnabled(true, true, this);
  // preDelayTimeSlider.onValueChange = [this] {};
  addAndMakeVisible(preDelayTimeLabel);
  preDelayTimeLabel.setText("Pre-delay", juce::dontSendNotification);
  preDelayTimeLabel.setJustificationType(juce::Justification::centred);
  preDelayTimeLabel.attachToComponent(&preDelayTimeSlider, false);
  preDelayTimeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(
      audioProcessor.apvts, "PreDelayTime", preDelayTimeSlider);
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
}

void ConekoAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  const int leftMargin = 15;
  const int topMargin = 20;
  const int bottomMargin = 20;
  const int dialWidth = 80;
  const int dialHeight = 90;
  openIRFileButton.setBounds(leftMargin, topMargin, dialWidth * 3, 40);
  irFileLabel.setBounds(leftMargin, topMargin + 40, dialWidth * 3, 20);
  inputGainSlider.setBounds(leftMargin, getHeight() - bottomMargin - dialHeight,
                            dialWidth, dialHeight);
  outputGainSlider.setBounds(leftMargin + dialWidth,
                             getHeight() - bottomMargin - dialHeight, dialWidth,
                             dialHeight);
  dryWetMixSlider.setBounds(leftMargin + dialWidth * 2,
                            getHeight() - bottomMargin - dialHeight, dialWidth,
                            dialHeight);
  decayTimeSlider.setBounds(leftMargin + dialWidth * 3,
                            getHeight() - bottomMargin - dialWidth * 4 +
                                dialHeight,
                            dialWidth * 3, dialWidth * 4 - dialHeight);
  preDelayTimeSlider.setBounds(getWidth() - leftMargin - dialWidth * 3,
                               dialHeight, dialWidth, dialHeight);
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
        audioProcessor.rawIRBuffer.setSize(
            static_cast<int>(reader->numChannels),
            static_cast<int>(reader->lengthInSamples));
        reader->read(&audioProcessor.rawIRBuffer, 0,
                     static_cast<int>(reader->lengthInSamples), 0, true, true);
        audioProcessor.loadImpulseResponse();
      }
    }
  });
}
