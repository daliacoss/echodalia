#pragma once

#include <iostream>

#include "rack.hpp"

namespace echodalia {

const std::vector<NVGcolor> THEME_COLORS = { nvgRGB(0x20, 0x20, 0x20),
                                             nvgRGB(0x14, 0x14, 0x30),
                                             nvgRGB(0x3a, 0x00, 0x6b),
                                             nvgRGB(0x77, 0x00, 0x00),
                                             nvgRGB(0x5b, 0x6f, 0x60) };
const std::vector<std::string> THEME_NAMES = { "Black",
                                               "Navy",
                                               "Indigo",
                                               "Red",
                                               "Grey-green" };

struct EDModule : rack::Module
{
  /*
   * this should correspond to an index from THEME_COLORS,
   * or -1 to use the default theme
   */
  int panelTheme = -1;

  virtual float getInputOrParamVal(int input, int param);

  virtual float getInputOrParamVal(int input,
                                   int param,
                                   bool& isInputConnected,
                                   bool useDisplayVal = false);

  virtual rack::simd::float_4 getParamVal4(int param1st,
                                           bool useDisplayVal = false);

  virtual rack::simd::float_4 getInputOrParamVal4(int input1st,
                                                  int param1st,
                                                  int& inputConnMask,
                                                  bool useDisplayVal = false);
  json_t* dataToJson(json_t* root);
  json_t* dataToJson() override;
  void dataFromJson(json_t* root) override;
};

} // namespace echodalia

const rack::simd::float_4 FLOAT_4_ZERO = rack::simd::float_4::zero();
const rack::simd::float_4 FLOAT_4_MASK = rack::simd::float_4::mask();

bool
nvgColorEquals(NVGcolor a, NVGcolor b);

extern unsigned int defaultTheme;

// Declare the Plugin, defined in plugin.cpp
extern rack::Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern rack::Model* modelRonda;
extern rack::Model* modelJab;
