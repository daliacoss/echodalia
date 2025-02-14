#pragma once
#include <bitset>
#include <cstdio>
#include <iostream>
#include <stdexcept>

#include "EDModule.hpp"
#include "rack.hpp"
//
// Declare the Plugin, defined in plugin.cpp
extern rack::Plugin* pluginInstance;

struct CKSSHorizontal : app::SvgSwitch
{
  CKSSHorizontal()
  {
    shadow->opacity = 0.0;
    addFrame(
      Svg::load(asset::plugin(pluginInstance, "res/CKSSHorizontal_0.svg")));
    addFrame(
      Svg::load(asset::plugin(pluginInstance, "res/CKSSHorizontal_1.svg")));
  }
};

// Declare each Model, defined in each module source file
extern rack::Model* modelRonda;
