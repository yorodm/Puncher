#include "Puncher.h"
#include "IPlug_include_in_plug_src.h"
#include <IGraphicsStructs.h>
#include <algorithm>

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
      WDL_String buildInfoStr;
      GetBuildInfoStr(buildInfoStr, __DATE__, __TIME__);
      CreateEditor(pGraphics, buildInfoStr);
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

  mInputPeakSender.Reset(sr);
  mOutputPeakSender.Reset(sr);
}

void Puncher::OnIdle()
{
  mInputPeakSender.TransmitData(*this);
  mOutputPeakSender.TransmitData(*this);
}

#if IPLUG_EDITOR
void Puncher::CreateEditor(IGraphics* pGraphics, WDL_String buildInfoStr) {
    const IRECT bounds = pGraphics->GetBounds();
    const IRECT innerBounds = bounds.GetPadded(-10.f);

    // Horizontal split
    RectSplitter h(innerBounds);

    IRECT leftMeter  = h.TakeLeft(0.1f);
    IRECT rightMeter = h.TakeRight(0.1f);
    IRECT middle     = h.TakeRemaining();

    // Split middle into two equal sections
    RectSplitter midSplit(middle);

    IRECT sectionA = midSplit.TakeLeft(0.5f);
    IRECT sectionB = midSplit.TakeRemaining();

    // Padding
    float pad = innerBounds.W() * 0.01f;
    //sectionA = sectionA.GetPadded(-pad);
    //sectionB = sectionB.GetPadded(-pad);

    // Vertical split for section A
    RectSplitter aSplit(sectionA);

    IRECT titleSection    = aSplit.TakeTop(0.2f);
    IRECT envelopeSection = aSplit.TakeRemaining();

    // Vertical split for section B
    RectSplitter bSplit(sectionB);

    IRECT versionSection = bSplit.TakeTop(0.2f);
    IRECT slidersSection = bSplit.TakeRemaining();

    if (pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      pGraphics->GetControlWithTag(kCtrlTagTitle)->SetTargetAndDrawRECTs(titleSection);
      pGraphics->GetControlWithTag(kCtrlTagEnvelope)->SetTargetAndDrawRECTs(envelopeSection);
      pGraphics->GetControlWithTag(kCtrlTagInputMeter)->SetTargetAndDrawRECTs(leftMeter);
      pGraphics->GetControlWithTag(kCtrlTagOutputMeter)->SetTargetAndDrawRECTs(rightMeter);
      pGraphics->GetControlWithTag(kCtrlTagAttack)->SetTargetAndDrawRECTs(slidersSection.GetGridCell(0, 0, 1, 3).GetPadded(-8));
      pGraphics->GetControlWithTag(kCtrlTagSustain)->SetTargetAndDrawRECTs(slidersSection.GetGridCell(0, 1, 1, 3).GetPadded(-8));
      pGraphics->GetControlWithTag(kCtrlTagOutput)->SetTargetAndDrawRECTs(slidersSection.GetGridCell(0, 2, 1, 3).GetPadded(-8));
      pGraphics->GetControlWithTag(kCtrlTagVersionNumber)->SetTargetAndDrawRECTs(versionSection);
      return;
    }

    pGraphics->SetLayoutOnResize(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_LIGHT_GRAY);

    // Center panel controls
    pGraphics->AttachControl(new ITextControl(titleSection, "Puncher", IText(30)), kCtrlTagTitle);
    pGraphics->AttachControl(new EnvelopeDisplayControl(envelopeSection, kParamAttack, kParamSustain, kParamOutput), kCtrlTagEnvelope);

    // Meters
    pGraphics->AttachControl(mInputMeter = new IVLEDMeterControl<2>(leftMeter, "", DEFAULT_STYLE, EDirection::Vertical), kCtrlTagInputMeter);
    pGraphics->AttachControl(mOutputMeter = new IVLEDMeterControl<2>(rightMeter, "", DEFAULT_STYLE, EDirection::Vertical), kCtrlTagOutputMeter);

    // Right panel controls
    pGraphics->AttachControl(new ITextControl(versionSection, buildInfoStr.Get(), DEFAULT_TEXT.WithAlign(EAlign::Center).WithSize(10)), kCtrlTagVersionNumber);
    pGraphics->AttachControl(new IVSliderControl(slidersSection.GetGridCell(0, 0, 1, 3).GetPadded(-8), kParamAttack, "Attack"), kCtrlTagAttack);
    pGraphics->AttachControl(new IVSliderControl(slidersSection.GetGridCell(0, 1, 1, 3).GetPadded(-8), kParamSustain, "Sustain"), kCtrlTagSustain);
    pGraphics->AttachControl(new IVSliderControl(slidersSection.GetGridCell(0, 2, 1, 3).GetPadded(-8), kParamOutput, "Output"), kCtrlTagOutput);
}
#endif

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

  const int meterChans = std::min(nChans, 2);
  mInputPeakSender.ProcessBlock(inputs, nFrames, kCtrlTagInputMeter, meterChans, 0);
  mOutputPeakSender.ProcessBlock(outputs, nFrames, kCtrlTagOutputMeter, meterChans, 0);
}
#endif
