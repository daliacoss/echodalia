#include "rack.hpp"
#include "plugin.hpp"

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
protected:
  float value = 0.f;

public:
  std::string fontPath;
  char normalChar = '@';
  std::string text;

  CharacterDisplay();

  void drawLayer(const DrawArgs& args, int layer) override;
};

struct ParamSegmentDisplay : rack::Widget
{
private:
  float prevValue;

public:
  // std::function<float()> getValue = [=]() { return 0; };
  float value = 0;
  rack::Module* rackModule = nullptr;
  int paramId = 0;
  int length = 6; // including the decimal point
  // rack::FramebufferWidget* fb;
  CharacterDisplay* displayWidget = nullptr;

  ParamSegmentDisplay();

  std::string getText();
  void draw(const DrawArgs& args) override;
};

struct SolidRect : rack::Widget
{
  NVGcolor color;

  void draw(const DrawArgs& args) override;
};

struct EchodaliaPanel : rack::SvgPanel
{
  // NVGcolor backgroundColor;
  SolidRect* bgw;
  EchodaliaPanel();
  void setBackground(std::shared_ptr<rack::window::Svg> svg);
};

struct EchodaliaWidget : rack::ModuleWidget
{

public:
  void refreshPanelTheme();
  void appendContextMenu(rack::Menu* menu) override;
  void step() override;
};

} // namespace dalia

