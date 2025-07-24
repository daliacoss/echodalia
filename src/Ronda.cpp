#include <cmath>
#include <stdexcept>
#include <string>

#include "plugin.hpp"
#include "widgets.hpp"

using namespace rack;

struct Ronda : echodalia::EDModule
{

protected:
  /* phasors before phase offset is applied */
  double phasors[4];
  // dsp::ClockDivider debugDivider;

public:
  static const int PHASORS_LEN = 4;
  static constexpr float MAX_FREQ_BASE = 8.f;
  static constexpr float MAX_VOUT = 10.f;
  enum ParamId
  {
    RESET_PARAM,
    FREQ_PARAM,
    RATE1_PARAM,
    PHASE1_PARAM = RATE1_PARAM + PHASORS_LEN,
    // SYNC1_PARAM = PHASE1_PARAM + PHASORS_LEN,
    RUN_PARAM = PHASE1_PARAM + PHASORS_LEN,
    PARAMS_LEN
  };
  enum InputId
  {
    RUN_INPUT,
    RESET_INPUT,
    FREQ_INPUT,
    // RATE1_INPUT,
    // PHASE1_INPUT = RATE1_INPUT + PHASORS_LEN,
    SYNC1_INPUT,
    INPUTS_LEN = SYNC1_INPUT + PHASORS_LEN
  };
  enum OutputId
  {
    PHSR1_OUTPUT,
    CLK1_OUTPUT = PHSR1_OUTPUT + PHASORS_LEN,
    OUTPUTS_LEN = CLK1_OUTPUT + PHASORS_LEN
  };
  enum LightId
  {
    LIGHTS_LEN
  };
  dsp::ClockDivider lightDivider;
  dsp::SchmittTrigger runTrigger;
  dsp::SchmittTrigger resetTrigger;
  dsp::SchmittTrigger syncTriggers[PHASORS_LEN];
  dsp::PulseGenerator clockPulses[PHASORS_LEN];
  bool isOutputPoly;

  float getFreqBase()
  {
    ParamQuantity* pq = getParamQuantity(FREQ_PARAM);
    if (pq) {
      return pq->getDisplayValue();
    } else {
      return 0;
    }
  }

  float getFreqCV()
  {
    return std::pow(2, getInput(FREQ_INPUT).getNormalVoltage(0.f));
  }

  float getFreqMain()
  {
    float f = getParamQuantity(FREQ_PARAM)->getDisplayValue();
    float v = getInput(FREQ_INPUT).getNormalVoltage(0.f);
    float freq = f * std::pow(2, v);
    return freq;
  }

  double getPhasor(int i, bool safe = true)
  {
    if (safe && i >= PHASORS_LEN) {
      throw std::out_of_range("out of range");
    }
    return phasors[i];
  }

  simd::float_4 getFreqRatio()
  {
    simd::float_4 ratio = getParamVal4(RATE1_PARAM, true);

    return ratio;
  }

  simd::float_4 getPhase()
  {
    simd::float_4 phase = getParamVal4(PHASE1_PARAM, true);
    return phase;
  }

  bool isRunning()
  {
    // runTrigger.process(getInput(RUN_INPUT).getNormalVoltage(1.0), 0.1, 1.0);
    runTrigger.process(getInputOrParamVal(RUN_INPUT, RUN_PARAM));
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

    configSwitch(RUN_PARAM, 0.f, 1.f, 1.0f, "Run", { "Off", "On" });
    configInput(RUN_INPUT, "Run");
    configSwitch(RESET_PARAM, 0, 1, 0, "Sync all phasors", { "Off", "On" });
    configInput(RESET_INPUT, "Sync all phasors");
    configParam(FREQ_PARAM,
                0.f,
                1.f,
                0.5f,
                "Base frequency",
                " Hz",
                MAX_FREQ_BASE + 1,
                1.f,
                -1.f);
    configInput(FREQ_INPUT, "Base frequency modifier");

    for (int i = 0; i < PHASORS_LEN; i++) {
      std::string phsr_name = "Phasor " + std::to_string(i + 1);
      configInput(i + SYNC1_INPUT, phsr_name + " sync");
      configParam(i + PHASE1_PARAM, 0.f, 1.f, 0.f, phsr_name + " phase");
      configParam(i + RATE1_PARAM,
                  -1.f,
                  1.f,
                  0.f,
                  phsr_name + " rate",
                  "",
                  32.f,
                  1.f,
                  0.f);
      configOutput(PHSR1_OUTPUT + i, phsr_name);
      configOutput(CLK1_OUTPUT + i, phsr_name + " clock");
    }
  }

  void process(const ProcessArgs& args) override;
  // json_t* dataToJson() override;
  // void dataFromJson(json_t* root) override;
};

void
Ronda::process(const ProcessArgs& args)
{
  bool run = isRunning();
  bool reset = isResetting();
  float phsr_fl[4];

  if (reset) {
    for (int i = 0; i < PHASORS_LEN; i++) {
      phasors[i] = 0;
      phsr_fl[i] = 0.f;
      clockPulses[i].trigger();
    }
  } else if (run) {
    double delta = getFreqBase() * getFreqCV() * args.sampleTime;
    double ratio = 1;
    bool is_sync;
    for (int i = 0; i < PHASORS_LEN; i++) {
      is_sync = syncTriggers[i].processEvent(
                  getInput(SYNC1_INPUT + i).getVoltage(), 0.1f, 1.0f) ==
                dsp::SchmittTrigger::TRIGGERED;

      if (is_sync) {
        phasors[i] = 0;
        clockPulses[i].trigger();
      } else {
        ratio = (double)getParamQuantity(RATE1_PARAM + i)->getDisplayValue();
        phasors[i] += delta * ratio;
        if (phasors[i] >= 1.0) {
          phasors[i] = std::fmod(phasors[i], 1.0);
          clockPulses[i].trigger();
        }
      }
      phsr_fl[i] = (float)phasors[i];
    }
  } else {
    for (int i = 0; i < PHASORS_LEN; i++) {
      phsr_fl[i] = (float)phasors[i];
    }
  }

  simd::float_4 phsr_simd = simd::float_4::load(phsr_fl);
  phsr_simd = simd::fmod(phsr_simd + getPhase(), 1.f) * MAX_VOUT;
  phsr_simd.store(phsr_fl);
  for (int i = 0; i < PHASORS_LEN; i++) {
    clockPulses[i].process(args.sampleTime);
    getOutput(PHSR1_OUTPUT + i).setVoltage(phsr_fl[i]);
    getOutput(CLK1_OUTPUT + i).setVoltage(clockPulses[i].isHigh() * 10.f);
  }
}

struct RondaWidget : echodalia::EDModuleWidget
{
public:
  RondaWidget(Ronda* ronda)
  {
    constexpr float XG = 2.54;
    constexpr float YG = 2.141666667;
    int i;
    float x;

    setModule(ronda);
    echodalia::EDPanel* panel = createPanel<echodalia::EDPanel>(
      asset::plugin(pluginInstance, "res/panels/Ronda.svg"));
    setPanel(panel);

    addChild(createWidget<ScrewBlack>(Vec(0, 0)));
    addChild(createWidget<ScrewBlack>(Vec(11 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - 15)));
    addChild(createWidget<ScrewSilver>(
      Vec(11 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - 15)));

    echodalia::ParamSegmentDisplay* sd =
      createWidget<echodalia::ParamSegmentDisplay>(mm2px(Vec(20.2, 17 * YG)));
    sd->rackModule = ronda;
    sd->paramId = Ronda::FREQ_PARAM;
    sd->length = 6;
    addChild(sd);

    addParam(createParamCentered<RoundLargeBlackKnob>(
      mm2px(Vec(12 * XG, 10 * YG)), ronda, Ronda::FREQ_PARAM));

    for (i = 0, x = 3 * XG; i < Ronda::PHASORS_LEN; i++, x += 6 * XG) {
      /* all columns */
      addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(x, 24 * YG)), ronda, Ronda::RATE1_PARAM + i));
      addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(x, 32 * YG)), ronda, Ronda::PHASE1_PARAM + i));
      addInput(createInputCentered<PJ301MPort>(
        mm2px(Vec(x, 40 * YG)), ronda, Ronda::SYNC1_INPUT + i));
      addOutput(createOutputCentered<PJ301MPort>(
        mm2px(Vec(x, 48 * YG)), ronda, Ronda::PHSR1_OUTPUT + i));
      addOutput(createOutputCentered<PJ301MPort>(
        mm2px(Vec(x, 55 * YG)), ronda, Ronda::CLK1_OUTPUT + i));
      /* last column only */
      if (i == Ronda::PHASORS_LEN - 1) {
        addParam(createParamCentered<VCVButton>(
          mm2px(Vec(x - 3 * XG, 7 * YG)), ronda, Ronda::RESET_PARAM));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 7 * YG)), ronda, Ronda::RESET_INPUT));
      }
      /* 1st column only */
      else if (!i) {
        addParam(createParamCentered<CKSS>(
          mm2px(Vec(x + 3 * XG, 7 * YG)), ronda, Ronda::RUN_PARAM));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 7 * YG)), ronda, Ronda::RUN_INPUT));
        addInput(createInputCentered<PJ301MPort>(
          mm2px(Vec(x, 15 * YG)), ronda, Ronda::FREQ_INPUT));
      }
    }
    // setPanelTheme(ronda->panelTheme);
  }
};

Model* modelRonda = createModel<Ronda, RondaWidget>("Ronda");
