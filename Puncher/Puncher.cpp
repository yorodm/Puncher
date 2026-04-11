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

    // Three vertical columns: input meter | center | output meter
    const float meterWidth = 40.f;
    const IRECT inputMeterRect = innerBounds.GetFromLeft(meterWidth);
    const IRECT outputMeterRect = innerBounds.GetFromRight(meterWidth);
    const IRECT centerPanel = innerBounds;

    // Center panel: split into left (title + envelope) and right (version + sliders)
    const IRECT leftPanel = centerPanel.GetFromLeft(300).GetReducedFromLeft(meterWidth);
    const IRECT rightPanel = centerPanel.GetFromRight(220).GetReducedFromRight(meterWidth);

    // Left panel: title at top, envelope fills rest
    const IRECT titleBounds = leftPanel.GetFromTop(50).GetCentredInside(200, 40);
    const IRECT envelopeBounds = leftPanel;

    // Right panel: version at top, sliders below
    const IRECT versionBounds = rightPanel.GetFromTop(30).GetPadded(-5.f);
    const IRECT sliderPanel = rightPanel.GetFromBottom(rightPanel.H() - 35.f).GetPadded(-10);

    // Meter bounds
    const IRECT inputMeterBounds = inputMeterRect.GetPadded(-2.f, -5.f, -2.f, -5.f);
    const IRECT outputMeterBounds = outputMeterRect.GetPadded(-2.f, -5.f, -2.f, -5.f);

    if (pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      pGraphics->GetControlWithTag(kCtrlTagTitle)->SetTargetAndDrawRECTs(titleBounds);
      pGraphics->GetControlWithTag(kCtrlTagEnvelope)->SetTargetAndDrawRECTs(envelopeBounds);
      pGraphics->GetControlWithTag(kCtrlTagInputMeter)->SetTargetAndDrawRECTs(inputMeterBounds);
      pGraphics->GetControlWithTag(kCtrlTagOutputMeter)->SetTargetAndDrawRECTs(outputMeterBounds);
      pGraphics->GetControlWithTag(kCtrlTagAttack)->SetTargetAndDrawRECTs(sliderPanel.GetGridCell(0, 0, 1, 3).GetPadded(-8));
      pGraphics->GetControlWithTag(kCtrlTagSustain)->SetTargetAndDrawRECTs(sliderPanel.GetGridCell(0, 1, 1, 3).GetPadded(-8));
      pGraphics->GetControlWithTag(kCtrlTagOutput)->SetTargetAndDrawRECTs(sliderPanel.GetGridCell(0, 2, 1, 3).GetPadded(-8));
      pGraphics->GetControlWithTag(kCtrlTagVersionNumber)->SetTargetAndDrawRECTs(versionBounds);
      return;
    }

    pGraphics->SetLayoutOnResize(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_LIGHT_GRAY);

    // Center panel controls
    pGraphics->AttachControl(new ITextControl(titleBounds, "Puncher", IText(30)), kCtrlTagTitle);
    pGraphics->AttachControl(new EnvelopeDisplayControl(envelopeBounds, kParamAttack, kParamSustain, kParamOutput), kCtrlTagEnvelope);

    // Meters
    pGraphics->AttachControl(mInputMeter = new IVMeterControl<2>(inputMeterBounds, "", DEFAULT_STYLE, EDirection::Vertical, {"L", "R"}), kCtrlTagInputMeter);
    pGraphics->AttachControl(mOutputMeter = new IVMeterControl<2>(outputMeterBounds, "", DEFAULT_STYLE, EDirection::Vertical, {"L", "R"}), kCtrlTagOutputMeter);

    // Right panel controls
    pGraphics->AttachControl(new ITextControl(versionBounds, buildInfoStr.Get(), DEFAULT_TEXT.WithAlign(EAlign::Center).WithSize(10)), kCtrlTagVersionNumber);
    pGraphics->AttachControl(new IVSliderControl(sliderPanel.GetGridCell(0, 0, 1, 3).GetPadded(-8), kParamAttack, "Attack"), kCtrlTagAttack);
    pGraphics->AttachControl(new IVSliderControl(sliderPanel.GetGridCell(0, 1, 1, 3).GetPadded(-8), kParamSustain, "Sustain"), kCtrlTagSustain);
    pGraphics->AttachControl(new IVSliderControl(sliderPanel.GetGridCell(0, 2, 1, 3).GetPadded(-8), kParamOutput, "Output"), kCtrlTagOutput);
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
