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
class ConekoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Slider::Listener {
public:
  ConekoAudioProcessorEditor(ConekoAudioProcessor &);
  ~ConekoAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  void sliderValueChanged(juce::Slider *slider) override;

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  ConekoAudioProcessor &audioProcessor;

  juce::Slider inputLevelSlider;
  juce::Label inputLevelLabel;
  juce::Slider outputLevelSlider;
  juce::Label outputLevelLabel;
  juce::Slider wetMixSlider;
  juce::Label wetMixLabel;
  juce::Slider decayTimeSlider;
  juce::Label decayTimeLabel;

  juce::AudioFormatManager formatManager;
  std::unique_ptr<juce::FileChooser> fileChooser;
  juce::TextButton openIRFileButton;
  void openButtonClicked();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConekoAudioProcessorEditor)
};
