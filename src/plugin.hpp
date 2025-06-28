#pragma once

#include "rack.hpp"
#include "iostream"

const rack::simd::float_4 FLOAT_4_ZERO = rack::simd::float_4::zero();
const rack::simd::float_4 FLOAT_4_MASK = rack::simd::float_4::mask();

// Declare the Plugin, defined in plugin.cpp
extern rack::Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern rack::Model* modelRonda;
extern rack::Model* modelJab;
