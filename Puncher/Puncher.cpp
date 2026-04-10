#include "Puncher.h"
#include "IPlug_include_in_plug_src.h"
#include <algorithm>

#if IPLUG_EDITOR
#include "IControls.h"
#endif

Puncher::Puncher(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamAttack)->InitDouble("Attack", 0., -100., 100.0, 1.0, "%");
  GetParam(kParamSustain)->InitDouble("Sustain", 0., -100., 100.0, 1.0, "%");
  GetParam(kParamOutput)->InitDouble("Output", 0., -12., 6.0, 0.1, "dB");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT bounds = pGraphics->GetBounds();
    const IRECT innerBounds = bounds.GetPadded(-10.f);
    const IRECT titleBounds = innerBounds.GetGridCell(0, 0, 3, 1).GetCentredInside(200, 50);
    const IRECT controlBounds = innerBounds.GetFromBottom(180).GetPadded(10.f);
    const IRECT sliderBounds = controlBounds.GetGridCell(0, 0, 1, 3);
    const IRECT versionBounds = innerBounds.GetFromTRHC(300, 20);

    if (pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      pGraphics->GetControlWithTag(kCtrlTagAttack)->SetTargetAndDrawRECTs(sliderBounds.GetGridCell(0, 0, 1, 3));
      pGraphics->GetControlWithTag(kCtrlTagSustain)->SetTargetAndDrawRECTs(sliderBounds.GetGridCell(0, 1, 1, 3));
      pGraphics->GetControlWithTag(kCtrlTagOutput)->SetTargetAndDrawRECTs(sliderBounds.GetGridCell(0, 2, 1, 3));
      pGraphics->GetControlWithTag(kCtrlTagTitle)->SetTargetAndDrawRECTs(titleBounds);
      pGraphics->GetControlWithTag(kCtrlTagVersionNumber)->SetTargetAndDrawRECTs(versionBounds);
      return;
    }

    pGraphics->SetLayoutOnResize(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_LIGHT_GRAY);
    pGraphics->AttachControl(new IVSliderControl(sliderBounds.GetGridCell(0, 0, 1, 3).GetPadded(-10.f), kParamAttack, "Attack"), kCtrlTagAttack);
    pGraphics->AttachControl(new IVSliderControl(sliderBounds.GetGridCell(0, 1, 1, 3).GetPadded(-10.f), kParamSustain, "Sustain"), kCtrlTagSustain);
    pGraphics->AttachControl(new IVSliderControl(sliderBounds.GetGridCell(0, 2, 1, 3).GetPadded(-10.f), kParamOutput, "Output"), kCtrlTagOutput);
    pGraphics->AttachControl(new ITextControl(titleBounds, "Puncher", IText(30)), kCtrlTagTitle);
    WDL_String buildInfoStr;
    GetBuildInfoStr(buildInfoStr, __DATE__, __TIME__);
    pGraphics->AttachControl(new ITextControl(versionBounds, buildInfoStr.Get(), DEFAULT_TEXT.WithAlign(EAlign::Far)), kCtrlTagVersionNumber);
  };
#endif
}

void Puncher::OnReset()
{
  const double sr = GetSampleRate();
  mA0Env1 = 1.0 - exp(-30.0 / sr);
  mB1Env1 = -exp(-30.0 / sr);
  mA0Env2 = 1.0 - exp(-1250.0 / sr);
  mB1Env2 = -exp(-1250.0 / sr);
  mA0Env3 = 1.0 - exp(-3.0 / sr);
  mB1Env3 = -exp(-3.0 / sr);

  mTmpEnv1 = 0.0;
  mTmpEnv2 = 0.0;
  mTmpEnv3 = 0.0;
}

#if IPLUG_DSP
void Puncher::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{ 
  const int nChans = std::min(NInChansConnected(), NOutChansConnected());
  if (nChans <= 0) {
    return;
  }

  const double attack = GetParam(kParamAttack)->Value() / 100.0;
  const double sustain = GetParam(kParamSustain)->Value() / 50.0;
  const double vol = pow(2.0, GetParam(kParamOutput)->Value() / 6.0);

  for (int s = 0; s < nFrames; s++) {
    const double spl0 = inputs[0][s];
    const double spl1 = (nChans > 1) ? inputs[1][s] : spl0;
    const double maxSpls = std::max(fabs(spl0), fabs(spl1));

    mTmpEnv1 = mA0Env1 * maxSpls - mB1Env1 * mTmpEnv1;
    const double env1 = sqrt(mTmpEnv1);
    mTmpEnv2 = mA0Env2 * maxSpls - mB1Env2 * mTmpEnv2;
    const double env2 = sqrt(mTmpEnv2);
    mTmpEnv3 = mA0Env3 * maxSpls - mB1Env3 * mTmpEnv3;
    const double env3 = sqrt(mTmpEnv3);

    const double safeEnv1 = env1 > 1e-20 ? env1 : 1.0;
    const double ratio2 = std::max(env2 / safeEnv1, 1.0);
    const double ratio3 = std::max(env3 / safeEnv1, 1.0);
    const double gain = vol * pow(ratio2, attack) * pow(ratio3, sustain);

    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
