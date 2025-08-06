#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>

#include "plugin.hpp"
#include "widgets.hpp"

namespace echodalia {
CharacterDisplay::CharacterDisplay()
{
  fontPath = rack::asset::plugin(pluginInstance,
                                 "res/9-segment-black/9-segment-black.ttf");
}

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

ParamSegmentDisplay::ParamSegmentDisplay()
{
  displayWidget =
    rack::createWidget<echodalia::CharacterDisplay>(rack::math::Vec(0, 0));
  addChild(displayWidget);
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
  }
  rack::Widget::draw(args);
}

void
SolidRect::draw(const DrawArgs& args)
{
  nvgFillColor(args.vg, color);
  nvgBeginPath(args.vg);
  nvgRect(args.vg, 0.0, 0.0, box.size.x, box.size.y);
  nvgFill(args.vg);
}

std::vector<int>
DotMatrixGridDisplay::pxToCellCoords(rack::Vec pos)
{
  rack::Vec cell_size =
    rack::Vec(dotsPerCol * dotSize.x, dotsPerRow * dotSize.y);
  std::vector<int> coords;
  if (!(cell_size.x && cell_size.y)) {
    return coords;
  }

  float divisor = ((dotsPerCol + dotsBetweenCols) * dotSize.x);
  float col = std::floor(pos.x / divisor);
  if (std::fmod(pos.x, divisor) >= cell_size.x) {
    return coords;
  }

  divisor = ((dotsPerRow + dotsBetweenRows) * dotSize.y);
  float row = std::floor(pos.y / divisor);
  if (std::fmod(pos.y, divisor) >= cell_size.y) {
    return coords;
  }

  coords = { (int)col, (int)row };
  return coords;
}

void
DotMatrixGridDisplay::draw(const DrawArgs& args)
{
  float w = dotsPerCol * dotSize.x;
  float h = dotsPerRow * dotSize.y;
  float x;
  float y;
  NVGcolor fill;
  std::vector<int> cell;
  int total_dots_per_col = dotsPerCol + dotsBetweenCols;
  int total_dots_per_row = dotsPerRow + dotsBetweenRows;

  for (const auto& p : cells) {
    cell = p.first;
    CellState state = p.second;
    if (!(state & ENABLED)) {
      continue;
    }
    x = total_dots_per_col * dotSize.x * cell.front();
    y = total_dots_per_row * dotSize.y * cell.back();
    fill = (state & PLAYING) ? playingColor : activeColor;
    nvgBeginPath(args.vg);
    nvgFillColor(args.vg, fill);
    nvgRect(args.vg, x, y, w, h);
    nvgFill(args.vg);
  }

  for (unsigned int d = columnDividers, i = 1; d > 0; d >>= 1, i++) {
    if (d % 2) {
      nvgBeginPath(args.vg);
      nvgFillColor(args.vg, activeColor);
      nvgRect(args.vg,
              dotSize.x * ((total_dots_per_col)*i - (dotsBetweenCols / 2) - 1),
              dotSize.y,
              dotSize.x,
              box.size.y - (dotSize.y * 2));
      nvgFill(args.vg);
    }
  }
}

void
DotMatrixGridDisplay::setBoxSizeInDots(float w, float h)
{
  box.size.x = w * dotSize.x;
  box.size.y = h * dotSize.y;
}

void
DotMatrixGridDisplay::onButton(const ButtonEvent& e)
{
  OpaqueWidget::onButton(e);
  if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
    std::vector<int> cell_coords = pxToCellCoords(e.pos);
    // if ((cell_coords.size() == 2) && pressCallback) {
    if (pressCallback) {
      if (cell_coords.size() == 2) {
        pressCallback(e, cell_coords.front(), cell_coords.back());
      } else {
        pressCallback(e, -1, -1);
      }
    }
  }
}

void
DotMatrixGridDisplay::onDragHover(const DragHoverEvent& e)
{
  OpaqueWidget::onDragHover(e);
  if (e.origin == this && e.button == GLFW_MOUSE_BUTTON_LEFT &&
      (e.mouseDelta.x || e.mouseDelta.y)) {
    std::vector<int> cell_coords = pxToCellCoords(e.pos);
    if (dragHoverCallback) {
      if (cell_coords.size() == 2) {
        dragHoverCallback(e, cell_coords.front(), cell_coords.back());
      } else {
        dragHoverCallback(e, -1, -1);
      }
    }
  }
  e.consume(this);
}

EDPanel::EDPanel()
{
  rack::SvgPanel();
  bgw = new SolidRect;
  bgw->color = THEME_COLORS[0];
  fb->addChildBottom(bgw);
}

void
EDPanel::setBackground(std::shared_ptr<rack::window::Svg> svg)
{
  rack::SvgPanel::setBackground(svg);
  bgw->box.size = fb->box.size;
}

void
EDModuleWidget::refreshPanelTheme()
{
  EDModule* edm = getModule<EDModule>();
  if (!edm || edm->panelTheme >= (int)THEME_COLORS.size()) {
    return;
  }
  NVGcolor color;
  if (edm->panelTheme < 0) {
    color = THEME_COLORS[defaultTheme];
  } else {
    color = THEME_COLORS[edm->panelTheme];
  }

  EDPanel* panel = dynamic_cast<EDPanel*>(getPanel());
  if (panel && panel->bgw && !nvgColorEquals(panel->bgw->color, color)) {
    panel->bgw->color = color;
    panel->fb->setDirty();
  }
}

void
EDModuleWidget::appendContextMenu(rack::Menu* menu)
{
  EDModule* edm = getModule<EDModule>();
  std::vector<std::string> names_w_default = THEME_NAMES;
  names_w_default.insert(names_w_default.begin(), "Default");

  menu->addChild(new rack::MenuSeparator);
  menu->addChild(rack::createIndexSubmenuItem(
    "Theme",
    names_w_default,
    [=]() { return edm->panelTheme + 1; },
    [=](int t) { edm->panelTheme = t - 1; }));
  menu->addChild(rack::createIndexSubmenuItem(
    "Default theme",
    THEME_NAMES,
    [=]() { return defaultTheme; },
    [=](int t) { defaultTheme = t; }));
}

void
EDModuleWidget::step()
{
  refreshPanelTheme();
  rack::ModuleWidget::step();
}

} // namespace echodalia
