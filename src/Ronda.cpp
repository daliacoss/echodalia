#include "plugin.hpp"
#include <cmath>

using namespace rack;

struct Ronda : EDModule
{

protected:
  double leader;
  /* phasors before phase offset is applied */
  double phasors[4];
  // dsp::ClockDivider debugDivider;

public:
  static const int PHASORS_LEN = 4;
  static constexpr float MAX_FREQ_BASE = 80.f;
  static constexpr float MAX_VOUT = 10.f;
  enum ParamId
  {
    RUN_PARAM,
    RESET_PARAM,
    FREQ_PARAM,
    RATE1_PARAM,
    PHASE1_PARAM = RATE1_PARAM + PHASORS_LEN,
    SYNC1_PARAM = PHASE1_PARAM + PHASORS_LEN,
    PARAMS_LEN = SYNC1_PARAM + PHASORS_LEN
  };
  enum InputId
  {
    RUN_INPUT,
    RESET_INPUT,
    FREQ_INPUT,
    RATE1_INPUT,
    PHASE1_INPUT = RATE1_INPUT + PHASORS_LEN,
    SYNC1_INPUT = PHASE1_INPUT + PHASORS_LEN,
    INPUTS_LEN = SYNC1_INPUT + PHASORS_LEN
  };
  enum OutputId
  {
    PHSR1_OUTPUT,
    OUTPUTS_LEN = PHASORS_LEN
  };
  enum LightId
  {
    RUN_LIGHT,
    RESET_LIGHT,
    LIGHTS_LEN
  };
  dsp::ClockDivider lightDivider;
  dsp::SchmittTrigger runTrigger;
  dsp::SchmittTrigger resetTrigger;
  dsp::TSchmittTrigger<simd::float_4> syncTrigger;
  dsp::BooleanTrigger resetBtnTrigger;
  bool isOutputPoly;

  float getFreqMain()
  {
    float f = getParamQuantity(FREQ_PARAM)->getDisplayValue();
    float v = getInput(FREQ_INPUT).getNormalVoltage(0.f);
    float freq = f * std::pow(2, v);
    return freq;
  }

  double getLeader() { return leader; }

  double getPhasor(int i, bool safe = true)
  {
    if (safe && i >= PHASORS_LEN) {
      throw std::out_of_range("out of range");
    }
    return phasors[i];
  }

  using EDModule::getInputOrParamVal;
  float getInputOrParamVal(int input,
                           int param,
                           bool& isInputConnected,
                           bool useDisplayVal = false) override
  {
    isInputConnected = false;
    if (param == RATE1_PARAM || param == SYNC1_PARAM) {
      return 1.f;
    } else if (param == PHASE1_PARAM) {
      return 0.f;
    }
    return EDModule::getInputOrParamVal(
      input, param, isInputConnected, useDisplayVal);
  }

  simd::float_4 getFreqRatio()
  {
    int input_conn_mask = 0;
    simd::float_4 ratio =
      getInputOrParamVal4(RATE1_INPUT, RATE1_PARAM, input_conn_mask, true);
    /* convert control voltages to ratios */
    ratio = ifelse(
      simd::movemaskInverse<simd::float_4>(input_conn_mask),
      // simd::clamp(simd::pow(32, simd::rescale(ratio, -5, 5, -1, 1)), -1, 1),
      simd::pow(32, simd::clamp(simd::rescale(ratio, -5, 5, -1, 1), -1, 1)),
      ratio);
    return ratio;
  }

  simd::float_4 getPhase()
  {
    int input_conn_mask = 0;
    simd::float_4 phase =
      getInputOrParamVal4(PHASE1_INPUT, PHASE1_PARAM, input_conn_mask, true);
    phase = ifelse(simd::movemaskInverse<simd::float_4>(input_conn_mask),
                   simd::clamp(simd::rescale(phase, 0.f, 10.f, 0.f, 1.f)),
                   phase);
    return phase;
  }

  simd::float_4 isSync()
  {
    simd::float_4 sync_params = {
      1.f,
      getInputOrParamVal(SYNC1_INPUT + 1, SYNC1_PARAM + 1),
      getInputOrParamVal(SYNC1_INPUT + 2, SYNC1_PARAM + 2),
      getInputOrParamVal(SYNC1_INPUT + 3, SYNC1_PARAM + 3)
    };
    syncTrigger.process(sync_params, 0.1f, 1.f);
    return syncTrigger.isHigh();
  }

  bool isRunning()
  {
    runTrigger.process(getInputOrParamVal(RUN_INPUT, RUN_PARAM), 0.1f, 1.f);
    return runTrigger.isHigh();
  }

  bool isResetting()
  {
    return resetTrigger.process(getInput(RESET_INPUT).getNormalVoltage(0) +
                                getParam(RESET_PARAM).getValue());
  }

  Ronda()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    /* main params and inputs */
    configSwitch(RUN_PARAM, 0.f, 1.f, 0.0f, "Run", { "Off", "On" });
    configInput(RUN_INPUT, "Run");
    configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset all phasors");
    configInput(RESET_INPUT, "Reset all phasors");
    configParam(FREQ_PARAM,
                0.f,
                1.f,
                0.25f,
                "Lead frequency",
                " Hz",
                MAX_FREQ_BASE + 1,
                1.f,
                -1.f);
    configInput(FREQ_INPUT, "Lead frequency");
    configLight(RESET_LIGHT, "Resetting");
    configLight(RUN_LIGHT, "Running");

    for (int i = 0; i < PHASORS_LEN; i++) {
      std::string phsr_name = "Phasor " + std::to_string(i + 1);
      /* common params, inputs, and outputs */
      configSwitch(
        i + SYNC1_PARAM, 0, 1, 0, phsr_name + " sync to 1", { "Off", "On" });
      configInput(i + SYNC1_INPUT, phsr_name + " sync");
      configParam(i + PHASE1_PARAM, 0.f, 1.f, 0.f, phsr_name + " phase");
      configInput(i + PHASE1_INPUT, phsr_name + " phase");
      configParam(i + RATE1_PARAM,
                  -1.f,
                  1.f,
                  0.f,
                  phsr_name + " rate",
                  "",
                  32.f,
                  1.f,
                  0.f);
      configInput(i + RATE1_INPUT, phsr_name + " rate");
      configOutput(PHSR1_OUTPUT + i, phsr_name);
    }

    // debugDivider.setDivision(48000);
  }

  void process(const ProcessArgs& args) override;
  json_t* dataToJson() override;
  void dataFromJson(json_t* root) override;
};

void
Ronda::process(const ProcessArgs& args)
{
  bool run = isRunning();
  bool reset = isResetting();
  float phsr_fl[4];
  simd::float_4 phsr_simd = 0;

  if (reset) {
    leader = 0;
    for (int i = 0; i < PHASORS_LEN; i++) {
      phasors[i] = 0;
      phsr_fl[i] = 0.f;
    }
  } else if (run) {
    double delta = getFreqMain() * args.sampleTime;
    double ratio = 1;
    float ratio_fl = 1.f;
    bool is_connected;
    int is_sync = simd::movemask(isSync());
    leader = std::fmod(leader + delta, 1);
    for (int i = 0; i < PHASORS_LEN; i++, is_sync >>= 1) {
      ratio_fl = getInputOrParamVal(
        RATE1_INPUT + i, RATE1_PARAM + i, is_connected, true);

      if (is_connected) {
        ratio = std::pow(32,
                         (double)math::clamp(
                           math::rescale(ratio_fl, -5, 5, -1, 1), -1.f, 1.f));
      } else {
        ratio = (double)ratio_fl;
      }

      if (is_sync % 2) {
        phasors[i] = leader * ratio;
      } else {
        phasors[i] += delta * ratio;
      }

      phsr_fl[i] = (float)phasors[i];
    }
  } else {
    for (int i = 0; i < PHASORS_LEN; i++) {
      phsr_fl[i] = (float)phasors[i];
    }
  }

  getLight(RESET_LIGHT).setBrightnessSmooth(reset, args.sampleTime);
  phsr_simd = simd::float_4::load(phsr_fl);
  phsr_simd = simd::fmod(phsr_simd + getPhase(), 1.f) * MAX_VOUT;

  if (isOutputPoly) {
    getOutput(PHSR1_OUTPUT).setChannels(PHASORS_LEN);
    getOutput(PHSR1_OUTPUT).setVoltageSimd(phsr_simd, 0);
  } else {
    phsr_simd.store(phsr_fl);
    getOutput(PHSR1_OUTPUT).setChannels(1);
    for (int i = 0; i < OUTPUTS_LEN; i++) {
      getOutput(PHSR1_OUTPUT + i).setVoltage(phsr_fl[i]);
    }
  }
}

json_t*
Ronda::dataToJson()
{
  json_t* root = json_object();
  json_object_set_new(root, "isOutputPoly", json_boolean(isOutputPoly));
  return root;
}

void
Ronda::dataFromJson(json_t* root)
{
  json_t* val = json_object_get(root, "isOutputPoly");
  if (val) {
    isOutputPoly = json_boolean_value(val);
  }
}

struct RondaWidget : ModuleWidget
{

  static constexpr float XG = 2.54;
  static constexpr float YG = 2.141666667;

  RondaWidget(Ronda* ronda)
  {
    int i;
    float x;

    setModule(ronda);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Ronda.svg")));

    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(1 * XG, YG))));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(23 * XG, YG))));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(1 * XG, 59 * YG))));
    addChild(createWidgetCentered<ScrewSilver>(mm2px(Vec(23 * XG, 59 * YG))));

    for (i = 0, x = 3 * XG; i < Ronda::PHASORS_LEN; i++, x += 6 * XG) {
      /* common params, inputs, and outputs */
      addOutput(createOutputCentered<DarkPJ301MPort>(
        mm2px(Vec(x, 55.5 * YG)), ronda, Ronda::PHSR1_OUTPUT + i));
      /* subphasor-only params */
      if (i) {
        addParam(createParamCentered<RoundBlackKnob>(
          mm2px(Vec(x, 15 * YG)), ronda, Ronda::RATE1_PARAM + i));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 20 * YG)), ronda, Ronda::RATE1_INPUT + i));
        addParam(createParamCentered<CKSSHorizontal>(
          mm2px(Vec(x, 29 * YG)), ronda, Ronda::SYNC1_PARAM + i));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 34 * YG)), ronda, Ronda::SYNC1_INPUT + i));
        addParam(createParamCentered<RoundBlackKnob>(
          mm2px(Vec(x, 43 * YG)), ronda, Ronda::PHASE1_PARAM + i));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 48 * YG)), ronda, Ronda::PHASE1_INPUT + i));
      }
      /* main-only params and inputs*/
      else {
        ParamWidget *pw = createParamCentered<VCVSlider>(
          mm2px(Vec(0,0)), ronda, Ronda::FREQ_PARAM);
        TransformWidget* tw = new TransformWidget;
        // tw->rotate(0.001);
        tw->setPosition(mm2px(Vec(x, 11 * YG)));
        tw->addChild(pw);
        addChild(tw);
        // addParam(createParamCentered<VCVSlider>(
          // mm2px(Vec(x, 11 * YG)), ronda, Ronda::FREQ_PARAM));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 20 * YG)), ronda, Ronda::FREQ_INPUT));
        addParam(createLightParamCentered<VCVLightButton<SmallSimpleLight<WhiteLight>>>(
          mm2px(Vec(x, 29 * YG)),
          ronda,
          Ronda::RESET_PARAM,
          Ronda::RESET_LIGHT));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 34 * YG)), ronda, Ronda::RESET_INPUT));
        addParam(createParamCentered<CKSS>(
          mm2px(Vec(x, 43 * YG)), ronda, Ronda::RUN_PARAM));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 48 * YG)), ronda, Ronda::RUN_INPUT));
      }
    }
  }

  void appendContextMenu(Menu* menu) override
  {
    Ronda* ronda = getModule<Ronda>();
    menu->addChild(new MenuSeparator);
    menu->addChild(
      createBoolPtrMenuItem("Polyphonic output", "", &ronda->isOutputPoly));
  }
};

Model* modelRonda = createModel<Ronda, RondaWidget>("Ronda");
