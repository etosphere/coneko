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
  setSize(500, 300);

  // set AudioFormatManager for reading IR file
  formatManager.registerBasicFormats();

  addAndMakeVisible(openIRFileButton);
  openIRFileButton.setButtonText("Open IR File...");
  openIRFileButton.onClick = [this] { openButtonClicked(); };

  addAndMakeVisible(inputLevelSlider);
  inputLevelSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  inputLevelSlider.setRange(-96.0f, 36.0f, 0.01f);
  inputLevelSlider.setSkewFactorFromMidPoint(0.0f);
  inputLevelSlider.setTextValueSuffix(" dB");
  inputLevelSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // inputLevelSlider.setPopupDisplayEnabled(true, true, this);
  inputLevelSlider.onValueChange = [this] {};
  inputLevelSlider.addListener(this);
  addAndMakeVisible(inputLevelLabel);
  inputLevelLabel.setText("Input", juce::dontSendNotification);
  inputLevelLabel.setJustificationType(juce::Justification::centred);
  inputLevelLabel.attachToComponent(&inputLevelSlider, false);

  addAndMakeVisible(outputLevelSlider);
  outputLevelSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  outputLevelSlider.setRange(-96.0f, 36.0f, 0.01f);
  outputLevelSlider.setSkewFactorFromMidPoint(0.0f);
  outputLevelSlider.setTextValueSuffix(" dB");
  outputLevelSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // outputLevelSlider.setPopupDisplayEnabled(true, true, this);
  outputLevelSlider.onValueChange = [this] {};
  outputLevelSlider.addListener(this);
  addAndMakeVisible(outputLevelLabel);
  outputLevelLabel.setText("Output", juce::dontSendNotification);
  outputLevelLabel.setJustificationType(juce::Justification::centred);
  outputLevelLabel.attachToComponent(&outputLevelSlider, false);

  addAndMakeVisible(wetMixSlider);
  wetMixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  wetMixSlider.setRange(0.0f, 100.0f, 1.0f);
  wetMixSlider.setTextValueSuffix(" %");
  wetMixSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow,
                               false, 60, 15);
  // wetMixSlider.setPopupDisplayEnabled(true, true, this);
  wetMixSlider.onValueChange = [this] {};
  wetMixSlider.addListener(this);
  addAndMakeVisible(wetMixLabel);
  wetMixLabel.setText("Mix", juce::dontSendNotification);
  wetMixLabel.setJustificationType(juce::Justification::centred);
  wetMixLabel.attachToComponent(&wetMixSlider, false);

  addAndMakeVisible(decayTimeSlider);
  decayTimeSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  decayTimeSlider.setRange(0.0f, 16.0f, 0.001f);
  decayTimeSlider.setSkewFactorFromMidPoint(3.0f);
  decayTimeSlider.setTextValueSuffix(" s");
  decayTimeSlider.setTextBoxStyle(
      juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
  // decayTimeSlider.setPopupDisplayEnabled(true, true, this);
  decayTimeSlider.onValueChange = [this] {};
  decayTimeSlider.addListener(this);
  addAndMakeVisible(decayTimeLabel);
  decayTimeLabel.setText("Decay", juce::dontSendNotification);
  decayTimeLabel.setJustificationType(juce::Justification::centred);
  decayTimeLabel.attachToComponent(&decayTimeSlider, false);

  inputLevelSlider.setValue(0.0f);
  outputLevelSlider.setValue(0.0f);
  wetMixSlider.setValue(100.0f);
  decayTimeSlider.setValue(3.0f);
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
  // g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30,
  // juce::Justification::centred, 1);
}

void ConekoAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  // sets the position and size of the slider with arguments
  // (x, y, width, height)

  const int leftMargin = 10;
  const int topMargin = 20;
  const int bottomMargin = 20;
  const int dialWidth = 80;
  const int dialHeight = 90;
  openIRFileButton.setBounds(leftMargin, topMargin, dialWidth * 3, 40);
  inputLevelSlider.setBounds(leftMargin,
                             getHeight() - bottomMargin - dialHeight, dialWidth,
                             dialHeight);
  outputLevelSlider.setBounds(leftMargin + dialWidth,
                              getHeight() - bottomMargin - dialHeight,
                              dialWidth, dialHeight);
  wetMixSlider.setBounds(leftMargin + dialWidth * 2,
                         getHeight() - bottomMargin - dialHeight, dialWidth,
                         dialHeight);
  decayTimeSlider.setBounds(leftMargin + dialWidth * 3,
                            getHeight() - bottomMargin - dialWidth * 4 +
                                dialHeight,
                            dialWidth * 3, dialWidth * 4 - dialHeight);
}

void ConekoAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {}

void ConekoAudioProcessorEditor::openButtonClicked() {
  fileChooser = std::make_unique<juce::FileChooser>(
      "Choose a WAV or AIFF File...", juce::File(), "*.wav;*.aiff", true,
      false);
  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;
  fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file != juce::File()) {
      auto *reader = formatManager.createReaderFor(file);
      if (reader != nullptr) {
        audioProcessor.impulseResponseBuffer.setSize(
            (int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&audioProcessor.impulseResponseBuffer, 0,
                     (int)reader->lengthInSamples, 0, true, true);
        DBG(audioProcessor.impulseResponseBuffer.getRMSLevel(0, 0, reader->lengthInSamples));
      }
    }
  });
}
