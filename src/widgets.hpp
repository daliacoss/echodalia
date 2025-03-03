#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>

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
  char normalChar = '@';
  std::string text;

  CharacterDisplay()
  {
    fontPath = rack::asset::plugin(pluginInstance,
                                   "res/9-segment-black/9-segment-black.ttf");
  }

  void drawLayer(const DrawArgs& args, int layer) override;
};

struct FloatSegmentDisplay : rack::Widget
{
private:
  float prevValue;

public:
  std::function<float()> getValue;
  int length = 6; // including the decimal point
  // rack::FramebufferWidget* fb;
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

  std::string getText();
  void draw(const DrawArgs& args) override;
};

} // namespace dalia

