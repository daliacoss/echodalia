#include <string>

#include "RondaEx.hpp"
#include "plugin.hpp"
#include "widgets.hpp"

using namespace rack;

RondaEx::RondaEx()
{
  config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
  for (int i = 0; i < PHASORS_LEN; i++) {
    std::string phsr_name = "Phasor " + std::to_string(i + 1);
    configParam(
      i + START_PARAM, -10.f, 10.f, 0.f, phsr_name + " start voltage");
    configParam(i + END_PARAM, -10.f, 10.f, 10.f, phsr_name + " end voltage");
    configInput(i + START_INPUT, phsr_name + " start voltage");
    configInput(i + END_INPUT, phsr_name + " end voltage");
  }
  configOutput(PHSR_POLY_OUTPUT, "Polyphonic phasor");
  configOutput(CLK_POLY_OUTPUT, "Polyphonic clock");

  getOutput(PHSR_POLY_OUTPUT).setChannels(4);

  getLeftExpander().producerMessage = &message[0];
  getLeftExpander().consumerMessage = &message[1];
}

void
RondaEx::process(const ProcessArgs& args)
{
  RondaExMessage* msg = (RondaExMessage*)getLeftExpander().consumerMessage;
  getOutput(PHSR_POLY_OUTPUT).setChannels(4);
  getOutput(PHSR_POLY_OUTPUT).setVoltageSimd(msg->phasor, 0);
  getOutput(CLK_POLY_OUTPUT).setChannels(4);
  getOutput(CLK_POLY_OUTPUT).setVoltageSimd(msg->clock, 0);
}

struct RondaExWidget : echodalia::EDModuleWidget
{
  RondaExWidget(RondaEx* ronda_ex)
  {
    constexpr float XG = 1.27000;
    constexpr float YG = 2.141666667;

    setModule(ronda_ex);
    echodalia::EDPanel* panel = createPanel<echodalia::EDPanel>(
      asset::plugin(pluginInstance, "res/panels/RondaEx.svg"));
    setPanel(panel);
    addChild(createWidget<ScrewBlack>(Vec(2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(
      Vec(0 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - 15)));
    addChild(createWidget<ScrewSilver>(
      Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - 15)));

    float xa = 5 * XG;
    float xb = 15 * XG;
    float ya = 3 * YG;
    float yb = 7.25 * YG;
    for (int i = 0; i < RondaEx::PHASORS_LEN; i++) {
      addParam(createParamCentered<RoundSmallBlackKnob>(
        mm2px(Vec(xa, ya)), ronda_ex, RondaEx::START_PARAM + i));
      addParam(createParamCentered<RoundSmallBlackKnob>(
        mm2px(Vec(xb, ya)), ronda_ex, RondaEx::END_PARAM + i));
      addInput(createInputCentered<PJ301MPort>(
        mm2px(Vec(xa, yb)), ronda_ex, RondaEx::START_INPUT + i));
      addInput(createInputCentered<PJ301MPort>(
        mm2px(Vec(xb, yb)), ronda_ex, RondaEx::END_INPUT + i));
      ya += 12 * YG;
      yb += 12 * YG;
    }
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(xa, ya)), ronda_ex, RondaEx::PHSR_POLY_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(xb, ya)), ronda_ex, RondaEx::CLK_POLY_OUTPUT));
  }
};

Model* modelRondaEx = createModel<RondaEx, RondaExWidget>("RondaEx");
