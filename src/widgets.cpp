#include "widgets.hpp"

namespace dalia {
void
CharacterDisplay::drawLayer(const DrawArgs& args, int layer)
{
  if (layer == 1) {
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

std::string
ParamSegmentDisplay::getText()
{
  std::stringstream text;
  if (length <= 0) {
    return text.str();
  }
  float _value = std::abs(value);
  int lg10 = std::log10(_value);
  int precision = length - 2;
  if (lg10 >= length) {
    _value = (int)_value % (int)std::pow(10, length);
    lg10 = std::log10(_value);
  }
  if (lg10 > 0) {
    precision = std::max(precision - lg10, 0);
  }

  text << std::fixed << std::setw(length - 2) << std::setprecision(precision)
       << _value;
  return text.str();
}

void
ParamSegmentDisplay::draw(const DrawArgs& args)
{
  float newValue = value;
  if (rackModule) {
    newValue = rackModule->getParamQuantity(paramId)->getDisplayValue();
  }
  if (newValue != value) {
    value = newValue;
    if (displayWidget) {
      displayWidget->text = getText();
    }
    // fb->setDirty();
  }
  rack::Widget::draw(args);
}

} // namespace dalia
