#include "plugin.hpp"

rack::Plugin* pluginInstance;

namespace echodalia {

float
EDModule::getInputOrParamVal(int input, int param)
{
  bool foo;
  return getInputOrParamVal(input, param, foo);
}

float
EDModule::getInputOrParamVal(int input,
                             int param,
                             bool& isInputConnected,
                             bool useDisplayVal)
{
  isInputConnected = false;
  rack::Input port = getInput(input);
  if (!port.isConnected()) {
    return useDisplayVal ? getParamQuantity(param)->getDisplayValue()
                         : getParam(param).getValue();
  }
  isInputConnected = true;
  return port.getVoltage();
}

rack::simd::float_4
EDModule::getParamVal4(int param1st, bool useDisplayVal)
{
  float raw_vals[4];
  for (int i = 0; i < 4; i++) {
    if (useDisplayVal) {
      raw_vals[i] = getParamQuantity(param1st + i)->getDisplayValue();
    } else {
      raw_vals[i] = getParam(param1st + i).getValue();
    }
  }
  return rack::simd::float_4::load(raw_vals);
}

rack::simd::float_4
EDModule::getInputOrParamVal4(int input1st,
                              int param1st,
                              int& inputConnMask,
                              bool useDisplayVal)
{
  bool is_input_conn = false;
  float raw_vals[4];
  for (int i = 0; i < 4; i++) {
    raw_vals[i] = getInputOrParamVal(
      input1st + i, param1st + i, is_input_conn, useDisplayVal);
    if (is_input_conn) {
      inputConnMask = (1 << i) + inputConnMask;
    }
  }
  return rack::simd::float_4::load(raw_vals);
}

json_t*
EDModule::dataToJson(json_t* root)
{
  json_object_set_new(root, "theme", json_integer(panelTheme));
  return root;
}

json_t*
EDModule::dataToJson()
{
  return dataToJson(json_object());
}

void
EDModule::dataFromJson(json_t* root)
{
  json_t* val = json_object_get(root, "theme");
  if (val) {
    panelTheme = json_integer_value(val);
    // std::cout << "set panel theme to " << panelTheme << "\n";
  }
}
} // namespace echodalia

bool
nvgColorEquals(NVGcolor a, NVGcolor b)
{
  return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
}

json_t*
settingsToJson()
{
  json_t* root = json_object();
  json_object_set_new(root, "defaultTheme", json_integer(defaultTheme));
  return root;
}

void
settingsFromJson(json_t* root)
{
  json_t* val = json_object_get(root, "defaultTheme");
  if (val) {
    defaultTheme = json_integer_value(val);
  }
}

void
init(rack::Plugin* p)
{
  pluginInstance = p;

  // Add modules here
  p->addModel(modelRonda);
  p->addModel(modelRondaEx);
  p->addModel(modelJab);
  p->addModel(modelAgate);
}

unsigned int defaultTheme = 0;
