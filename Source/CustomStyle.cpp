#include "CustomStyle.h"

namespace juce {

CustomStyle::CustomStyle() {
  setDefaultSansSerifTypeface(juce::Typeface::createSystemTypefaceFor(
      BinaryData::SpartanMedium_ttf, BinaryData::SpartanMedium_ttfSize));

  auto backgroundColour = Colour::fromRGB(252, 248, 237);
  auto normalTextColour = Colour::fromRGB(111, 76, 91);
  auto lightTextColour = Colour::fromRGB(158, 119, 119);
  auto buttonColour = Colour::fromRGB(158, 119, 119);

  auto sliderFillColour = Colour::fromRGB(158, 119, 119);
  auto sliderOutlineColour = Colour::fromRGB(222, 186, 157);

  setColour(ColourScheme::widgetBackground, normalTextColour);
  setColour(ColourScheme::windowBackground, backgroundColour);

  setColour(Slider::textBoxOutlineColourId, backgroundColour);
  setColour(Slider::textBoxBackgroundColourId, backgroundColour);
  setColour(Slider::textBoxTextColourId, lightTextColour);
  setColour(Slider::textBoxHighlightColourId, lightTextColour);
  setColour(Label::textColourId, normalTextColour);
  setColour(TextButton::buttonColourId, buttonColour);
  setColour(ToggleButton::textColourId, normalTextColour);
  setColour(ToggleButton::tickColourId, lightTextColour);
  setColour(ToggleButton::tickDisabledColourId, sliderOutlineColour);
  setColour(Slider::rotarySliderFillColourId, sliderFillColour);
  setColour(Slider::rotarySliderOutlineColourId, sliderOutlineColour);
  setColour(Slider::thumbColourId, sliderFillColour);
}

CustomStyle::~CustomStyle() {}

void CustomStyle::drawRotarySlider(Graphics &g, int x, int y, int width,
                                   int height, float sliderPos,
                                   float rotaryStartAngle, float rotaryEndAngle,
                                   Slider &slider) {
  auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
  auto fill = slider.findColour(Slider::rotarySliderFillColourId);

  auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);

  auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto toAngle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
  auto lineW = jmin(4.0f, radius * 0.5f);
  auto arcRadius = radius - lineW * 0.5f;

  // dial background path
  Path backgroundArc;
  backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                              arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                              rotaryEndAngle, true);

  g.setColour(outline);
  g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved,
                                             PathStrokeType::rounded));

  if (slider.isEnabled()) {
    // dial fill path
    Path valueArc;
    valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius,
                           arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

    g.setColour(fill);
    g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved,
                                          PathStrokeType::rounded));
  }

  // bar
  Point<float> thumbPoint(
      bounds.getCentreX() +
          (arcRadius - 4.0f) * std::cos(toAngle - MathConstants<float>::halfPi),
      bounds.getCentreY() +
          (arcRadius - 4.0f) *
              std::sin(toAngle - MathConstants<float>::halfPi));

  g.setColour(slider.findColour(Slider::thumbColourId));
  g.drawLine(backgroundArc.getBounds().getCentreX(),
             backgroundArc.getBounds().getCentreY(), thumbPoint.getX(),
             thumbPoint.getY(), lineW);
}

void CustomStyle::drawButtonBackground(Graphics &g, Button &button,
                                       const Colour &backgroundColour,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown) {
  auto cornerSize = 6.0f;
  auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

  auto baseColour =
      backgroundColour
          .withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

  if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
    baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

  g.setColour(baseColour);

  auto flatOnLeft = button.isConnectedOnLeft();
  auto flatOnRight = button.isConnectedOnRight();
  auto flatOnTop = button.isConnectedOnTop();
  auto flatOnBottom = button.isConnectedOnBottom();

  if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom) {
    Path path;
    path.addRoundedRectangle(
        bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
        cornerSize, cornerSize, !(flatOnLeft || flatOnTop),
        !(flatOnRight || flatOnTop), !(flatOnLeft || flatOnBottom),
        !(flatOnRight || flatOnBottom));

    g.fillPath(path);

    g.setColour(button.findColour(ComboBox::outlineColourId));
    g.strokePath(path, PathStrokeType(1.0f));
  } else {
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(button.findColour(ComboBox::outlineColourId));
    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) {
      g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
  }
}

} // namespace juce