#include <functional>

#include "plugin.hpp"
#include "rack.hpp"

namespace echodalia {
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
  float value = 0;
  rack::Module* rackModule = nullptr;
  int paramId = 0;
  int length = 6; // including the decimal point
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

struct DotMatrixGridDisplay : rack::OpaqueWidget
{
public:
  enum CellState
  {
    ENABLED = 1,
    PLAYING = 2,
    SELECTED = 4
  };
  int dotsPerCol = 4;
  int dotsPerRow = 4;
  int dotsBetweenCols = 3;
  int dotsBetweenRows = 1;
  uint32_t columnDividers = 0;
  NVGcolor activeColor = nvgRGB(0x86, 0x86, 0x86);
  NVGcolor playingColor = nvgRGB(0xFF, 0xFF, 0xFF);
  rack::Vec dotSize = rack::mm2px(rack::Vec(1.07083, 1.07083));
  std::map<std::vector<int>, CellState> cells;
  std::function<void(const ButtonEvent&, int, int)> pressCallback;
  std::function<void(const DragHoverEvent&, int, int)> dragHoverCallback;

  std::vector<int> pxToCellCoords(rack::Vec pos);
  void setBoxSizeInDots(float w, float h);
  void onButton(const ButtonEvent& e) override;
  void onDragHover(const DragHoverEvent& e) override;
  void draw(const DrawArgs& args) override;
};

struct EDPanel : rack::SvgPanel
{
  SolidRect* bgw;
  EDPanel();
  void setBackground(std::shared_ptr<rack::window::Svg> svg);
};

struct EDModuleWidget : rack::ModuleWidget
{
  void refreshPanelTheme();
  void appendContextMenu(rack::Menu* menu) override;
  void step() override;
};

} // namespace echodalia

