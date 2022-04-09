/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
/**
 */
class ConekoAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  using APVTS = juce::AudioProcessorValueTreeState;

  ConekoAudioProcessorEditor(ConekoAudioProcessor &);
  ~ConekoAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  ConekoAudioProcessor &audioProcessor;

  juce::AudioFormatManager formatManager;
  std::unique_ptr<juce::FileChooser> fileChooser;
  juce::TextButton openIRFileButton;
  juce::Label irFileLabel;

  juce::Slider inputGainSlider;
  juce::Label inputGainLabel;
  std::unique_ptr<APVTS::SliderAttachment> inputGainSliderAttachment;
  juce::Slider outputGainSlider;
  juce::Label outputGainLabel;
  std::unique_ptr<APVTS::SliderAttachment> outputGainSliderAttachment;
  juce::Slider dryWetMixSlider;
  juce::Label dryWetMixLabel;
  std::unique_ptr<APVTS::SliderAttachment> dryWetMixSliderAttachment;
  juce::Slider decayTimeSlider;
  juce::Label decayTimeLabel;
  std::unique_ptr<APVTS::SliderAttachment> decayTimeSliderAttachment;
  juce::Slider preDelayTimeSlider;
  juce::Label preDelayTimeLabel;
  std::unique_ptr<APVTS::SliderAttachment> preDelayTimeSliderAttachment;

  void openButtonClicked();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConekoAudioProcessorEditor)
};
