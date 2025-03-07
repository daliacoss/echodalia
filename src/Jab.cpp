#include "Echodalia.hpp"
#include "plugin.hpp"

using namespace rack;

struct Jab : dalia::Echodalia
{
protected:
  dsp::SchmittTrigger inputTrigger;
  dsp::BooleanTrigger buttonTrigger;
  dsp::BooleanTrigger combinedGateTrigger;
  dsp::PulseGenerator gateStartPulse;
  dsp::PulseGenerator gateEndPulse;
  dsp::ClockDivider lightDivider;

public:
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
    MOMENTARY_LIGHT,
    NOT_MOMENTARY_LIGHT,
    LATCH_LIGHT,
    NOT_LATCH_LIGHT,
    START_LIGHT,
    END_LIGHT,
    START_OR_END_LIGHT,
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

  float highVoltageOut = 10.f;
  float lowVoltageOut = 0.f;
  float pulseLength = 0.001f;
  bool isLatchHigh = false;
  int gateSource = INPUT_IF_CONNECTED_ELSE_BUTTON;
  float lightFadeoutLambda = 10.f;

  Jab()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(GATE_PARAM, 0, 1, 0, "Gate", { "Off", "On" });
    configInput(GATE_INPUT, "Gate");
    configOutput(MOMENTARY_OUTPUT, "Momentary gate");
    configOutput(NOT_MOMENTARY_OUTPUT, "Inverted momentary gate");
    configOutput(LATCH_OUTPUT, "Latch gate");
    configOutput(NOT_LATCH_OUTPUT, "Inverted latch gate");
    configOutput(START_OUTPUT, "Momentary high trigger");
    configOutput(END_OUTPUT, "Momentary low trigger");
    configOutput(START_OR_END_OUTPUT, "Momentary high/low trigger");
    buttonTrigger.s = dsp::BooleanTrigger::LOW;
    lightDivider.setDivision(16);
  }

  void process(const ProcessArgs& args) override;
};

void
Jab::process(const ProcessArgs& args)
{
  buttonTrigger.processEvent(getParam(GATE_PARAM).getValue());
  inputTrigger.processEvent(getInput(GATE_INPUT).getNormalVoltage(0.f));
  bool gate_state;
  switch (gateSource) {
    case INPUT_IF_CONNECTED_ELSE_BUTTON:
      if (getInput(GATE_INPUT).isConnected()) {
        gate_state = inputTrigger.isHigh();
      } else {
        gate_state = buttonTrigger.isHigh();
      }
      break;
    case BUTTON_ONLY:
      gate_state = buttonTrigger.isHigh();
      break;
    case INPUT_ONLY:
      gate_state = inputTrigger.isHigh();
      break;
    case BUTTON_AND_INPUT:
      gate_state = buttonTrigger.isHigh() && inputTrigger.isHigh();
      break;
    case BUTTON_OR_INPUT:
      gate_state = buttonTrigger.isHigh() || inputTrigger.isHigh();
      break;
    default:
      gate_state = false;
  }

  int gate_event = combinedGateTrigger.processEvent(gate_state);
  if (gate_event == dsp::BooleanTrigger::TRIGGERED) {
    gateStartPulse.trigger(pulseLength);
    isLatchHigh = !isLatchHigh;
  } else if (gate_event == dsp::BooleanTrigger::UNTRIGGERED) {
    gateEndPulse.trigger(pulseLength);
  }

  // PulseGenerator::process gives us the pulse state *before* advancing
  bool is_start = gateStartPulse.process(args.sampleTime);
  bool is_end = gateEndPulse.process(args.sampleTime);
  bool is_mom = (is_start || combinedGateTrigger.isHigh());
  int flags = is_mom + (!is_mom << 1) + (isLatchHigh << 2) +
              (!isLatchHigh << 3) + (is_start << 4) + (is_end << 5) +
              ((is_start | is_end) << 6);
  bool is_light_active = lightDivider.process();

  for (int i = 0; i < OUTPUTS_LEN && i < LIGHTS_LEN; i++) {
    bool is_high = flags & (1 << i);
    getOutput(i).setVoltage(is_high ? highVoltageOut : lowVoltageOut);
    if (is_light_active) {
      getLight(i).setBrightnessSmooth(is_high,
                                      args.sampleTime *
                                        lightDivider.getDivision(),
                                      lightFadeoutLambda);
    }
  }
}

struct JabWidget : ModuleWidget
{
  static constexpr float XG = 2.54;
  static constexpr float YG = 2.141666667;

  JabWidget(Jab* jab)
  {
    int i;
    float x = 3 * XG;
    float y;

    setModule(jab);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Jab.svg")));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(x, 1.1 * YG))));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(x, 59 * YG))));

    addParam(
      createParamCentered<CKD6>(mm2px(Vec(x, 8 * YG)), jab, Jab::GATE_PARAM));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(x, 13 * YG)), jab, Jab::GATE_INPUT));

    for (i = 0, y = 19 * YG; i < Jab::OUTPUTS_LEN; i++, y += 6 * YG) {
      addOutput(createOutputCentered<DarkPJ301MPort>(mm2px(Vec(x, y)), jab, i));
      addChild(createLightCentered<TinyLight<GreenLight>>(
        mm2px(Vec(5 * XG, y)), jab, i));
    }
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
  }
};

Model* modelJab = createModel<Jab, JabWidget>("Jab");
