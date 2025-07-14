#include "plugin.hpp"
#include "widgets.hpp"
#include <algorithm>
#include <string>
using namespace rack;

struct Jab : echodalia::EDModule
{
protected:
  // dsp::SchmittTrigger inputTrigger;
  dsp::TSchmittTrigger<simd::float_4> inputTriggers[4];
  // unsigned int gateState[16] = { 0 };
  // unsigned int lastGateState[16] = { 0 };
  simd::float_4 lastGates[4];
  simd::float_4 latches[4];
  simd::float_4 gateStartPulses[4];
  simd::float_4 gateEndPulses[4];
  dsp::BooleanTrigger resetButtonTrigger;
  // dsp::BooleanTrigger combinedGateTrigger;
  // dsp::PulseGenerator gateStartPulse;
  // dsp::PulseGenerator gateEndPulse;
  dsp::ClockDivider lightDivider;

public:
  enum ParamId
  {
    GATE_PARAM,
    RESET_PARAM,
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
    OUTPUTS_LEN
  };
  /* LightId order must correspond with OutputId */
  enum LightId
  {
    MOMENTARY_LIGHT,
    NOT_MOMENTARY_LIGHT,
    LATCH_LIGHT,
    NOT_LATCH_LIGHT,
    START_LIGHT,
    END_LIGHT,
    LIGHTS_LEN
  };
  enum GateSource
  {
    INPUT_IF_CONNECTED_ELSE_BUTTON,
    BUTTON_ONLY,
    INPUT_ONLY,
    BUTTON_AND_INPUT,
    BUTTON_OR_INPUT
  };

  simd::float_4 highVoltageOut = { 10, 10, 10, 10 };
  simd::float_4 lowVoltageOut = FLOAT_4_ZERO;
  float pulseLength = 0.001f;
  // bool isLatchHigh = false;
  GateSource gateSource = INPUT_IF_CONNECTED_ELSE_BUTTON;
  float lightFadeoutLambda = 15.f;
  int numChannels = 0;

  Jab()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(GATE_PARAM, 0, 1, 0, "Gate", { "Off", "On" });
    configButton(RESET_PARAM, "Reset latches");
    configInput(GATE_INPUT, "Gate");
    configOutput(MOMENTARY_OUTPUT, "Momentary gate");
    configOutput(NOT_MOMENTARY_OUTPUT, "Inverted momentary gate");
    configOutput(LATCH_OUTPUT, "Latch gate");
    configOutput(NOT_LATCH_OUTPUT, "Inverted latch gate");
    configOutput(START_OUTPUT, "Low-to-high trigger");
    configOutput(END_OUTPUT, "High-to-low trigger");
    // configOutput(START_OR_END_OUTPUT, "Momentary high/low trigger");
    lightDivider.setDivision(8);

    for (int i = 0; i < 4; i++) {
      lastGates[i] = FLOAT_4_ZERO;
      latches[i] = FLOAT_4_ZERO;
      gateStartPulses[i] = FLOAT_4_ZERO;
      gateEndPulses[i] = FLOAT_4_ZERO;
    }
  }

  void process(const ProcessArgs& args) override;
  json_t* dataToJson() override;
  void dataFromJson(json_t* root) override;
};

void
Jab::process(const ProcessArgs& args)
{
  rack::Port gate_input = getInput(GATE_INPUT);
  int num_channels =
    numChannels ? numChannels : std::max(gate_input.getChannels(), 1);
  int channels_div4 = ((num_channels - 1) / 4) + 1;
  for (int i = 0; i < OUTPUTS_LEN; i++) {
    getOutput(i).setChannels(num_channels);
  }

  if (resetButtonTrigger.process(getParam(RESET_PARAM).getValue())) {
    for (int i = 0; i < 4; i++) {
      latches[i] = FLOAT_4_ZERO;
    }
  }

  simd::float_4 gate_button_mask =
    (getParam(GATE_PARAM).getValue()) ? FLOAT_4_MASK : FLOAT_4_ZERO;
  simd::float_4 gates[4];

  GateSource gate_source = gateSource;
  if (gate_source == INPUT_IF_CONNECTED_ELSE_BUTTON) {
    if (gate_input.isConnected()) {
      gate_source = INPUT_ONLY;
    } else {
      gate_source = BUTTON_ONLY;
    }
  }

  switch (gate_source) {
    case BUTTON_ONLY:
      for (int i = 0; i < 4; i++) {
        gates[i] = gate_button_mask;
      }
      break;
    case INPUT_ONLY:
      for (int i = 0; i < 4; i++) {
        inputTriggers[i].process(
          gate_input.getVoltageSimd<simd::float_4>(i * 4));
        gates[i] = inputTriggers[i].isHigh();
      }
      break;
    case BUTTON_AND_INPUT:
      for (int i = 0; i < 4; i++) {
        // gates[i] = gates[i] && gate_button.getValue();
        inputTriggers[i].process(
          gate_input.getVoltageSimd<simd::float_4>(i * 4));
        gates[i] = gate_button_mask & inputTriggers[i].isHigh();
      }
      break;
    case BUTTON_OR_INPUT:
      for (int i = 0; i < 4; i++) {
        inputTriggers[i].process(
          gate_input.getVoltageSimd<simd::float_4>(i * 4));
        gates[i] = gate_button_mask | inputTriggers[i].isHigh();
      }
      break;
    default:
      break;
  }

  simd::float_4 voltages[OUTPUTS_LEN];
  for (int i = 0, i4 = 0; i < 4; i++, i4 += 4) {
    gateStartPulses[i] = simd::ifelse(
      simd::andnot(lastGates[i], gates[i]),
      pulseLength,
      simd::fmax(FLOAT_4_ZERO, gateStartPulses[i] - args.sampleTime));
    gateEndPulses[i] = simd::ifelse(
      simd::andnot(gates[i], lastGates[i]),
      pulseLength,
      simd::fmax(FLOAT_4_ZERO, gateEndPulses[i] - args.sampleTime));
    latches[i] = simd::ifelse(
      simd::andnot(lastGates[i], gates[i]), ~latches[i], latches[i]);

    if (i < channels_div4) {
      voltages[START_OUTPUT] = simd::ifelse(
        gateStartPulses[i] > FLOAT_4_ZERO, highVoltageOut, lowVoltageOut);
      voltages[END_OUTPUT] = simd::ifelse(
        gateEndPulses[i] > FLOAT_4_ZERO, highVoltageOut, lowVoltageOut);
      voltages[LATCH_OUTPUT] =
        simd::ifelse(latches[i], highVoltageOut, lowVoltageOut);
      voltages[NOT_LATCH_OUTPUT] =
        simd::ifelse(latches[i], lowVoltageOut, highVoltageOut);
      voltages[MOMENTARY_OUTPUT] =
        simd::ifelse(gates[i], highVoltageOut, lowVoltageOut);
      voltages[NOT_MOMENTARY_OUTPUT] =
        simd::ifelse(gates[i], lowVoltageOut, highVoltageOut);

      for (int k = 0; k < OUTPUTS_LEN; k++) {
        getOutput(k).setVoltageSimd(voltages[k], i4);
      }

      if (!i && lightDivider.process()) {
        for (int k = 0; k < OUTPUTS_LEN && k < LIGHTS_LEN; k++) {
          getLight(k).setBrightnessSmooth(voltages[k][0] > 0,
                                          args.sampleTime *
                                            lightDivider.getDivision(),
                                          lightFadeoutLambda);
        }
      }
    }

    lastGates[i] = gates[i];
  }
}

json_t*
Jab::dataToJson()
{
  json_t* root = json_object();
  json_object_set_new(root, "numChannels", json_integer(numChannels));
  json_object_set_new(root, "gateSource", json_integer(gateSource));
  return echodalia::EDModule::dataToJson(root);
}

void
Jab::dataFromJson(json_t* root)
{
  json_t* val = json_object_get(root, "numChannels");
  if (val) {
    numChannels = json_integer_value(val);
  }
  val = json_object_get(root, "gateSource");
  if (val) {
    gateSource = (GateSource)json_integer_value(val);
  }
  echodalia::EDModule::dataFromJson(root);
}

struct JabWidget : echodalia::EDModuleWidget
{
  static constexpr float XG = 2.54;
  static constexpr float YG = 2.141666667;

  JabWidget(Jab* jab)
  {
    int i;
    float x = 3 * XG;
    float y;

    setModule(jab);
    echodalia::EDPanel* panel = createPanel<echodalia::EDPanel>(
      asset::plugin(pluginInstance, "res/panels/Jab.svg"));
    setPanel(panel);
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(1 * XG, 1.1 * YG))));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(5 * XG, 59 * YG))));

    addParam(
      createParamCentered<CKD6>(mm2px(Vec(x, 8 * YG)), jab, Jab::GATE_PARAM));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(x, 13 * YG)), jab, Jab::GATE_INPUT));
    addParam(createParamCentered<VCVButton>(
      mm2px(Vec(x, 18 * YG)), jab, Jab::RESET_PARAM));

    for (i = 0, y = 25 * YG; i < Jab::OUTPUTS_LEN; i++, y += 6 * YG) {
      addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, y)), jab, i));
      addChild(createLightCentered<SmallLight<RedLight>>(
        mm2px(Vec(XG, y + (2 * YG))), jab, i));
    }
    // setPanelTheme(jab->panelTheme);
  }

  void appendContextMenu(Menu* menu) override
  {
    Jab* jab = getModule<Jab>();
    menu->addChild(new MenuSeparator);
    menu->addChild(createIndexPtrSubmenuItem("Gate source",
                                             { "CV if connected, else button",
                                               "Button only",
                                               "CV only",
                                               "Button AND CV",
                                               "Button OR CV" },
                                             &jab->gateSource));
    menu->addChild(createIndexPtrSubmenuItem("Polyphony channels",
                                             { "Auto",
                                               "1",
                                               "2",
                                               "3",
                                               "4",
                                               "5",
                                               "6",
                                               "7",
                                               "8",
                                               "9",
                                               "10",
                                               "11",
                                               "12",
                                               "13",
                                               "14",
                                               "15",
                                               "16" },
                                             &jab->numChannels));
    echodalia::EDModuleWidget::appendContextMenu(menu);
  }
};

Model* modelJab = createModel<Jab, JabWidget>("Jab");
