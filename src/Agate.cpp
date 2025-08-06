#include "plugin.hpp"
#include "widgets.hpp"

using namespace rack;

struct Agate : echodalia::EDModule
{
private:
  float _position;
  float _globalGateLength;

public:
  static const int PATTERNS_LEN = 4;
  static const int STEPS_MAX = 32;
  enum ParamId
  {
    GATE_LENGTH_PARAM,
    PATTERN_MODE_PARAM,
    PARAMS_LEN
  };
  enum InputId
  {
    ADDRESS_INPUT,
    PATTERN_INPUT,
    GATE_LENGTH_INPUT = PATTERN_INPUT + PATTERNS_LEN,
    INPUTS_LEN = GATE_LENGTH_INPUT + PATTERNS_LEN
  };
  enum OutputId
  {
    GATE_OUTPUT,
    OUTPUTS_LEN = GATE_OUTPUT + PATTERNS_LEN
  };
  enum LightId
  {
    LIGHTS_LEN
  };
  enum PatternMode
  {
    ONE_CHANNEL,
    TWO_CHANNELS,
    FOUR_CHANNELS
  };
  uint8_t patterns[PATTERNS_LEN] = {0, 0, 0, 0};
  PatternMode patternMode;
  bool isMuteWhenZero = true;

  Agate();
  void process(const ProcessArgs& args) override;
  float getGlobalGateLength();
  int getNumChannels();
  float getPosition();
  void setGlobalGateLength(float);
  void setPosition(float);
  json_t* dataToJson() override;
  void dataFromJson(json_t* root) override;
};

Agate::Agate()
{
  config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
  configSwitch(
    PATTERN_MODE_PARAM,
    ONE_CHANNEL,
    FOUR_CHANNELS,
    FOUR_CHANNELS,
    "Pattern mode",
    { "1 channel, 32 steps", "2 channels, 16 steps", "4 channel, 8 steps" });
  configInput(ADDRESS_INPUT, "Address");
  configParam(
    GATE_LENGTH_PARAM, 0.0, 1.0, 1.0, "Default gate length", "%", 0.0, 100.0);
  for (int i = 0; i < PATTERNS_LEN; i++) {
    std::string ptrn_name = "Pattern " + std::to_string(i + 1);
    configInput(PATTERN_INPUT + i, ptrn_name);
    configInput(GATE_LENGTH_INPUT + i, ptrn_name + " gate length");
    configOutput(GATE_OUTPUT + i, ptrn_name + " gate");
  }
}

void
Agate::process(const ProcessArgs& args)
{
  for (int i = 0; i < PATTERNS_LEN; i++) {
    Input p = getInput(PATTERN_INPUT + i);
    if (p.isConnected()) {
      patterns[i] = (uint8_t)math::clamp(
        std::round((p.getVoltage() / 10.0) * 256.0), 0.f, 255.f);
    }
  }
  patternMode = (PatternMode)getParam(PATTERN_MODE_PARAM).getValue();
  setPosition(getInput(ADDRESS_INPUT).getVoltage() / 10);
  setGlobalGateLength(getParam(GATE_LENGTH_PARAM).getValue());

  int num_steps = STEPS_MAX / getNumChannels();
  int cur_step = getPosition() * num_steps;
  float step_length = 1.0 / num_steps;

  // prevent triggers being sent while position is exactly 0 (i.e., while the
  // phasor driving the sequencer is stopped)
  bool maybe_high =
    (!isMuteWhenZero || (getPosition() > 0.f)) &&
    (std::fmod((getPosition() / step_length), 1.0) <= getGlobalGateLength());

  for (int i = 0; i < PATTERNS_LEN; i += (num_steps / 8)) {
    int ptrn_id = i + (cur_step / 8);
    bool is_high = maybe_high && ((patterns[ptrn_id] >> (cur_step % 8)) % 2);
    getOutput(GATE_OUTPUT + i).setVoltage(10 * is_high);
  }
}

json_t*
Agate::dataToJson()
{
  json_t* root = json_object();
  json_t* ptrns = json_array();
  for (int i = 0; i < PATTERNS_LEN; i++) {
    json_array_append_new(ptrns, json_integer(patterns[i]));
  }
  json_object_set_new(root, "patterns", ptrns);
  json_object_set_new(root, "isMuteWhenZero", json_integer(isMuteWhenZero));
  return echodalia::EDModule::dataToJson(root);
}

void
Agate::dataFromJson(json_t* root)
{
  json_t* val = json_object_get(root, "patterns");
  json_t* ptrn;
  if (json_is_array(val)) {
    for (size_t i = 0; (i < json_array_size(val)) && (i < PATTERNS_LEN); i++) {
      ptrn = json_array_get(val, i);
      if (json_is_integer(ptrn)) {
        patterns[i] = json_integer_value(ptrn);
      }
    }
  }
  val = json_object_get(root, "isMuteWhenZero");
  if (json_is_integer(val)) {
    isMuteWhenZero = json_integer_value(val);
  }
  echodalia::EDModule::dataFromJson(root);
}

int
Agate::getNumChannels()
{
  switch (patternMode) {
    case ONE_CHANNEL:
      return 1;
    case TWO_CHANNELS:
      return 2;
    case FOUR_CHANNELS:
      return 4;
  }

  return 1;
}

float
Agate::getPosition()
{
  return _position;
}

void
Agate::setPosition(float v)
{
  // keep position within 0-1 range (mimic python modulus)
  _position = std::fmod(std::fmod(v, 1.0) + 1.0, 1.0);
}

float
Agate::getGlobalGateLength()
{
  return _globalGateLength;
}

void
Agate::setGlobalGateLength(float v)
{
  _globalGateLength = math::clamp(v, 0.0, 1.0);
}

struct AgateWidget : echodalia::EDModuleWidget
{
private:
  uint8_t _columns[Agate::PATTERNS_LEN];
  int _currentStep;
  int _numChannels;
  int _currentTouchAction = 0;

public:
  enum TouchAction
  {
    NONE = 0,
    DRAW,
    ERASE
  };
  echodalia::DotMatrixGridDisplay* dotMatrix;
  FramebufferWidget* fb;
  AgateWidget(Agate* agate);
  void step() override;
  void appendContextMenu(Menu* menu) override;
};

AgateWidget::AgateWidget(Agate* agate)
{
  setModule(agate);
  echodalia::EDPanel* panel = createPanel<echodalia::EDPanel>(
    asset::plugin(pluginInstance, "res/panels/Agate.svg"));
  setPanel(panel);

  addChild(createWidget<ScrewBlack>(Vec(0, 0)));
  addChild(createWidget<ScrewBlack>(Vec(8 * RACK_GRID_WIDTH, 0)));
  addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - 15)));
  addChild(
    createWidget<ScrewSilver>(Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - 15)));

  dotMatrix =
    createWidget<echodalia::DotMatrixGridDisplay>(mm2px(Vec(9.6, 35.5)));
  fb = new FramebufferWidget;
  fb->addChild(dotMatrix);
  addChild(fb);
  dotMatrix->setBoxSizeInDots(25, 39);

  dotMatrix->pressCallback = [=](const ButtonEvent& e, int col, int row) {
    Agate* agate = getModule<Agate>();
    if (!agate) {
      return;
    }
    if (col < 0 || row < 0) {
      // clicked margin or space between cells
      _currentTouchAction = NONE;
    } else {
      Port input = agate->getInput(Agate::PATTERN_INPUT + col);
      if (input.isConnected()) {
        // no point toggling cell that will be immediately reverted
        _currentTouchAction = NONE;
      } else {
        // toggle cell under cursor
        agate->patterns[col] ^= 1 << row;
        _currentTouchAction =
          ((agate->patterns[col] >> row) % 2) ? DRAW : ERASE;
      }
    }
  };

  dotMatrix->dragHoverCallback =
    [=](const DragHoverEvent& e, int col, int row) {
      Agate* agate = getModule<Agate>();
      if (!agate) {
        return;
      }
      if (col >= 0 && row >= 0) {
        Port input = agate->getInput(Agate::PATTERN_INPUT + col);
        if (input.isConnected()) {
          return;
        }
        switch (_currentTouchAction) {
          case NONE:
            agate->patterns[col] ^= 1 << row;
            _currentTouchAction =
              ((agate->patterns[col] >> row) % 2) ? DRAW : ERASE;
            break;
          case DRAW:
            agate->patterns[col] |= 1 << row;
            break;
          case ERASE:
            agate->patterns[col] &= ~((char)(1 << row));
            break;
        }
      }
    };

  SvgWidget* overlay = createWidget<SvgWidget>(mm2px(Vec(8.403, 34.431)));
  FramebufferWidget* overlay_fb = new FramebufferWidget;
  overlay->setSvg(rack::Svg::load(
    rack::asset::plugin(pluginInstance, "res/widgets/DotMatrix_overlay.svg")));
  overlay_fb->addChild(overlay);
  addChild(overlay_fb);

  addInput(createInputCentered<PJ301MPort>(
    mm2px(Vec(7.62, 21.4167)), agate, Agate::ADDRESS_INPUT));
  addParam(createParamCentered<RoundBlackKnob>(
    mm2px(Vec(22.86, 19.27503)), agate, Agate::GATE_LENGTH_PARAM));
  addParam(createParamCentered<CKSSThree>(
    mm2px(Vec(34.29, 19.27503)), agate, Agate::PATTERN_MODE_PARAM));
  for (int i = 0; i < Agate::PATTERNS_LEN; i++) {
    float x = 7.62 + (i * 10.161);
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(x, 85.6668)), agate, Agate::PATTERN_INPUT + i));
    addInput(createInputCentered<PJ301MPort>(
      mm2px(Vec(x, 100.65849)), agate, Agate::GATE_LENGTH_INPUT + i));
    addOutput(createOutputCentered<PJ301MPort>(
      mm2px(Vec(x, 117.79185)), agate, Agate::GATE_OUTPUT + i));
  }
}

void
AgateWidget::step()
{
  Agate* agate = getModule<Agate>();
  if (!(agate && dotMatrix && fb)) {
    echodalia::EDModuleWidget::step();
    return;
  }

  int num_channels = agate->getNumChannels();
  int num_steps = Agate::STEPS_MAX / num_channels;
  int cur_step = agate->getPosition() * num_steps;
  bool is_dirty =
    ((num_channels != _numChannels) || (cur_step != _currentStep));

  for (int i = 0, ptrn = 0; i < Agate::PATTERNS_LEN; i++) {
    ptrn = agate->patterns[i];
    is_dirty = is_dirty || (_columns[i] != ptrn);
    if (is_dirty) {
      for (int k = 0; k < 8; k++) {
        // this works as long as num_channels is 1, 2, or 4
        int step = (((i * 8) % num_steps) + k);
        bool is_cur_step = (cur_step == step);
        dotMatrix->cells[{ i, k }] =
          // set/unset ENABLED and PLAYING
          (echodalia::DotMatrixGridDisplay::CellState)((ptrn % 2) |
                                                       (is_cur_step * 2));
        ptrn >>= 1;
      }
    }
    _columns[i] = agate->patterns[i];
  }

  dotMatrix->columnDividers = (num_channels == 4)   ? 0b111
                              : (num_channels == 2) ? 0b010
                                                    : 0;

  _numChannels = num_channels;
  _currentStep = cur_step;

  fb->dirty = is_dirty;
  echodalia::EDModuleWidget::step();
}
void
AgateWidget::appendContextMenu(Menu* menu)
{
  Agate* agate = getModule<Agate>();
  menu->addChild(new MenuSeparator);
  menu->addChild(createBoolPtrMenuItem(
    "Mute while ADDR is 0 V", "", &agate->isMuteWhenZero));
  echodalia::EDModuleWidget::appendContextMenu(menu);
}

Model* modelAgate = createModel<Agate, AgateWidget>("Agate");
