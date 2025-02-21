#include <cmath>
#include <iomanip>
// #include <iostream>
#include <sstream>

#include "functional"
#include "plugin.hpp"
#include "rack.hpp"

namespace dalia {
struct CKSSHorizontal : rack::app::SvgSwitch
{
  CKSSHorizontal()
  {
    shadow->opacity = 0.0;
    addFrame(rack::Svg::load(
      rack::asset::plugin(pluginInstance, "res/CKSSHorizontal_0.svg")));
    addFrame(rack::Svg::load(
      rack::asset::plugin(pluginInstance, "res/CKSSHorizontal_1.svg")));
  }
};

struct CharacterDisplay : rack::Widget
{
private:
  float prevValue = 0.f;

public:
  std::string fontPath;
  // int length = 6; // including the decimal point
  char normalChar = '@';
  std::string text;

  CharacterDisplay()
  {
    fontPath = rack::asset::plugin(pluginInstance,
                                   "res/9-segment-black/9-segment-black.ttf");
  }

  void drawLayer(const DrawArgs& args, int layer) override
  {
    if (layer == 1) {
      // std::cout << "drawing CharacterDisplay\n";
      std::shared_ptr<rack::Font> font = APP->window->loadFont(fontPath);
      if (font) {
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, 16);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 20));
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
        nvgText(args.vg,
                0.0,
                0.0,
                std::string(text.length(), normalChar).c_str(),
                NULL);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 255));
        nvgText(args.vg, 0.0, 0.0, text.c_str(), NULL);
      }
    }
    rack::Widget::drawLayer(args, layer);
  }
};

struct FloatSegmentDisplay : rack::Widget
{
private:
  float prevValue;

public:
  std::function<float()> getValue;
  int length = 6; // including the decimal point
  rack::FramebufferWidget* fb;
  CharacterDisplay* displayWidget;

  FloatSegmentDisplay()
  {
    // fb = new rack::FramebufferWidget;
    // addChild(fb);
    displayWidget =
      rack::createWidget<dalia::CharacterDisplay>(rack::math::Vec(0, 0));
    // fb->box.size = rack::math::Vec(20,20);
    addChild(displayWidget);
    // fb->addChild(displayWidget);
  }

  std::string getText()
  {
    std::stringstream text;
    if (length <= 0) {
      return text.str();
    }
    float value = std::abs(getValue());
    int lg10 = std::log10(value);
    int precision = length - 2;
    if (lg10 >= length) {
      value = (int)value % (int)std::pow(10, length);
      lg10 = std::log10(value);
    }
    if (lg10 > 0) {
      precision = std::max(precision - lg10, 0);
    }

    text << std::fixed << std::setw(length - 2) << std::setprecision(precision)
         << value;
    return text.str();
  }

  void draw(const DrawArgs& args) override
  {
    float value = getValue(); // std::abs(getValue());
    if (value != prevValue) {
      prevValue = value;

      displayWidget->text = getText();
      // fb->setDirty();
    }
    rack::Widget::draw(args);
  }
};

} // namespace dalia

