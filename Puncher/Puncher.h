#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

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
  kCtrlTagEnvelope,
  kCtrlTagInputMeter,
  kCtrlTagOutputMeter
};

#if IPLUG_EDITOR
#include "IControls.h"
#include "EnvelopeDisplayControl.h"
#endif

using namespace iplug;
using namespace igraphics;

class Puncher final : public Plugin
{
public:
  Puncher(const InstanceInfo& info);

#if IPLUG_EDITOR
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
  void CreateEditor(IGraphics* pGraphics, WDL_String buildInfoStr);
#endif

  void OnReset() override;
  void OnIdle() override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

private:
  double mA0Env1 = 0.0;
  double mB1Env1 = 0.0;
  double mA0Env2 = 0.0;
  double mB1Env2 = 0.0;
  double mA0Env3 = 0.0;
  double mB1Env3 = 0.0;
  double mTmpEnv1 = 0.0;
  double mTmpEnv2 = 0.0;
  double mTmpEnv3 = 0.0;

  IPeakSender<2> mInputPeakSender;
  IPeakSender<2> mOutputPeakSender;
#if IPLUG_EDITOR
  IVMeterControl<2>* mInputMeter = nullptr;
  IVMeterControl<2>* mOutputMeter = nullptr;
#endif
};

#if IPLUG_EDITOR
class RectSplitter
{
public:
  RectSplitter(const IRECT& r)
  : mRemaining(r) {}

  // Take from left (consumes space)
  IRECT TakeLeft(float frac)
  {
    IRECT slice = mRemaining.FracRectHorizontal(frac, true);
    mRemaining = mRemaining.GetFromLeft(mRemaining.W() * frac);
    return slice;
  }

  // Take from right (consumes space)
  IRECT TakeRight(float frac)
  {
    IRECT slice = mRemaining.FracRectHorizontal(frac, false);
    mRemaining = mRemaining.GetFromRight(mRemaining.W() * frac);
    return slice;
  }

  // Take from top (consumes space)
  IRECT TakeTop(float frac)
  {
    IRECT slice = mRemaining.FracRectVertical(frac, true);
    mRemaining = mRemaining.GetFromTop(mRemaining.H() * frac);
    return slice;
  }

  // Take from bottom (consumes space)
  IRECT TakeBottom(float frac)
  {
    IRECT slice = mRemaining.FracRectVertical(frac, false);
    mRemaining = mRemaining.GetFromBottom(mRemaining.H() * frac);
    return slice;
  }

  // Take all remaining
  IRECT TakeRemaining()
  {
    return mRemaining;
  }

private:
  IRECT mRemaining;
};
#endif
