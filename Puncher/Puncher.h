#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kParamAttack = 0,
  kParamSustain,
  kParamOutput,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagVersionNumber = 0,
  kCtrlTagAttack,
  kCtrlTagSustain,
  kCtrlTagOutput,
  kCtrlTagTitle,
  kCtrlTagEnvelope
};
using namespace iplug;
using namespace igraphics;

#if IPLUG_EDITOR
#include "EnvelopeDisplayControl.h"
#endif



class PuncherEditor {
    public:
        void CreateEditor(IGraphics* pGraphics, WDL_String buildInfo);
};

class Puncher final : public Plugin
{
public:
  Puncher(const InstanceInfo& info);

#if IPLUG_EDITOR
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
#endif

  void OnReset() override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

private:
  PuncherEditor  pEditor = PuncherEditor();
  double mA0Env1 = 0.0;
  double mB1Env1 = 0.0;
  double mA0Env2 = 0.0;
  double mB1Env2 = 0.0;
  double mA0Env3 = 0.0;
  double mB1Env3 = 0.0;
  double mTmpEnv1 = 0.0;
  double mTmpEnv2 = 0.0;
  double mTmpEnv3 = 0.0;
};
