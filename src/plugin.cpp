#include "plugin.hpp"

rack::Plugin* pluginInstance;

namespace dalia {

float
Echodalia::getInputOrParamVal(int input, int param)
{
  bool foo;
  return getInputOrParamVal(input, param, foo);
}

float
Echodalia::getInputOrParamVal(int input,
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
Echodalia::getParamVal4(int param1st, bool useDisplayVal)
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
Echodalia::getInputOrParamVal4(int input1st,
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
Echodalia::dataToJson(json_t* root)
{
  json_object_set_new(root, "theme", json_integer(panelTheme));
  return root;
}

json_t*
Echodalia::dataToJson()
{
  return dataToJson(json_object());
}

void
Echodalia::dataFromJson(json_t* root)
{
  json_t* val = json_object_get(root, "theme");
  if (val) {
    panelTheme = json_integer_value(val);
    // std::cout << "set panel theme to " << panelTheme << "\n";
  }
}
} // namespace dalia

bool nvgColorEquals(NVGcolor a, NVGcolor b) {
  return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
}

void
init(rack::Plugin* p)
{
  pluginInstance = p;

  // Add modules here
  p->addModel(modelRonda);
  p->addModel(modelJab);
}
