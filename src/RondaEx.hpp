#include "plugin.hpp"

using namespace rack;

struct RondaExMessage
{
  simd::float_4 phasor;
  simd::float_4 clock;
};

struct RondaEx : echodalia::EDModule
{
public:
  static const int PHASORS_LEN = 4;
  enum ParamId
  {
    START_PARAM,
    END_PARAM = START_PARAM + PHASORS_LEN,
    PARAMS_LEN = END_PARAM + PHASORS_LEN
  };
  enum InputId
  {
    START_INPUT,
    END_INPUT = START_INPUT + PHASORS_LEN,
    INPUTS_LEN = END_INPUT + PHASORS_LEN
  };
  enum OutputId
  {
    PHSR_POLY_OUTPUT,
    CLK_POLY_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId
  {
    LIGHTS_LEN
  };

  RondaExMessage message[2] = {};

  RondaEx();
  void process(const ProcessArgs& args) override;
};

