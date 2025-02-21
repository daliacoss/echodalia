#include "rack.hpp"

namespace dalia {
struct Echodalia : rack::Module
{
  virtual float getInputOrParamVal(int input, int param)
  {
    bool foo;
    return getInputOrParamVal(input, param, foo);
  }

  virtual float getInputOrParamVal(int input,
                                   int param,
                                   bool& isInputConnected,
                                   bool useDisplayVal = false)
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

  virtual rack::simd::float_4 getInputOrParamVal4(int input1st,
                                                  int param1st,
                                                  int& inputConnMask,
                                                  bool useDisplayVal = false)
  {
    bool is_input_conn = false;
    float raw_vals[4];
    for (int i = 0; i < 4; i++) {
      raw_vals[i] = getInputOrParamVal(
        input1st + i, input1st + i, is_input_conn, useDisplayVal);
      if (is_input_conn) {
        inputConnMask = (1 << i) + inputConnMask;
      }
    }
    return rack::simd::float_4::load(raw_vals);
  }
};
} // namespace dalia
