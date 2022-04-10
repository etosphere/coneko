#pragma once

#include <JuceHeader.h>

namespace juce {

class CustomStyle : public juce::LookAndFeel_V4 {
public:
  CustomStyle();
  ~CustomStyle();

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;

  void drawButtonBackground(Graphics &g, Button &button,
                            const Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
};

} // namespace juce