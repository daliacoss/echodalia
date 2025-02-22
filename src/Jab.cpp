#include "Echodalia.hpp"
#include "plugin.hpp"

using namespace rack;

struct Jab : dalia::Echodalia
{
  enum ParamId
  {
    GATE_PARAM,
    PARAMS_LEN
  };
  enum InputId
  {
    GATE_INPUT,
    INPUTS_LEN
  };
  enum OutputId
  {
    MOMENTARY_OUTPUT,
    NOT_MOMENTARY_OUTPUT,
    LATCH_OUTPUT,
    NOT_LATCH_OUTPUT,
    START_OUTPUT,
    END_OUTPUT,
    START_OR_END_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId
  {
    LIGHTS_LEN
  };
  Jab()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(GATE_PARAM, 0, 1, 0, "Gate", { "Off", "On" });
  }
};

struct JabWidget : ModuleWidget
{
  static constexpr float XG = 2.54;
  static constexpr float YG = 2.141666667;

  JabWidget(Jab* jab)
  {
    setModule(jab);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Jab.svg")));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(1 * XG, 1.1 * YG))));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(1 * XG, 59 * YG))));
  }
};

Model* modelJab = createModel<Jab, JabWidget>("Jab");
