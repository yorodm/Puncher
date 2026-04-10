#include "EnvelopeDisplayControl.h"
#include "Puncher.h"

EnvelopeDisplayControl::EnvelopeDisplayControl(const IRECT& bounds, int attackParam, int sustainParam, int outputParam)
: IControl(bounds), mAttackParam(attackParam), mSustainParam(sustainParam), mOutputParam(outputParam)
{
  SetParamIdx(mAttackParam, 0);
  SetParamIdx(mSustainParam, 1);
  SetParamIdx(mOutputParam, 2);
}

void EnvelopeDisplayControl::OnResize()
{
  const float pad = 10.f;
  const float bottomPad = 30.f; // space for labels
  mPlotBounds = mRECT.GetPadded(-pad, -pad, -pad, -bottomPad);
  SetDirty(false);
}

void EnvelopeDisplayControl::Draw(IGraphics& g)
{
  // Background
  g.FillRect(COLOR_WHITE, mRECT, nullptr);
  g.DrawRect(COLOR_MID_GRAY, mRECT, nullptr, 1.f);

  Plugin* pPlug = static_cast<Plugin*>(GetDelegate());
  if (!pPlug) return;

  const double attackVal = pPlug->GetParam(mAttackParam)->Value();
  const double sustainVal = pPlug->GetParam(mSustainParam)->Value();
  const double outputVal = pPlug->GetParam(mOutputParam)->Value();

  // Normalize values to 0-1 range for visualization
  const float attackNorm = static_cast<float>((attackVal + 100.0) / 200.0);
  const float sustainNorm = static_cast<float>((sustainVal + 100.0) / 200.0);
  const float outputNorm = static_cast<float>((outputVal + 12.0) / 18.0);

  // Calculate envelope curve points
  const int numPoints = 100;

  // Build the path
  for (int i = 0; i < numPoints; i++)
  {
    const float t = static_cast<float>(i) / (numPoints - 1);
    float x = mPlotBounds.L + t * mPlotBounds.W();

    float y;
    if (t < 0.3f)
    {
      // Attack phase: ramp from 0 to peak
      const float attackT = t / 0.3f;
      const float peak = 0.2f + attackNorm * 0.8f;
      y = attackT * peak;
    }
    else if (t < 0.7f)
    {
      // Sustain phase: hold at sustain level
      const float peak = 0.2f + attackNorm * 0.8f;
      const float sustainLevel = sustainNorm * peak;
      y = sustainLevel;
    }
    else
    {
      // Output tail: decay based on output
      const float sustainLevel = sustainNorm * (0.2f + attackNorm * 0.8f);
      const float decayT = (t - 0.7f) / 0.3f;
      y = sustainLevel * (1.0f - decayT * 0.5f);
    }

    // Scale Y to bounds (invert: higher value = higher on screen)
    const float plotY = mPlotBounds.B - y * mPlotBounds.H();

    if (i == 0)
      g.PathMoveTo(x, plotY);
    else
      g.PathLineTo(x, plotY);
  }

  // Draw curve stroke with gradient
  g.PathStroke(IPattern::CreateLinearGradient(mPlotBounds, EDirection::Vertical, {{IColor(255, 66, 135, 245), 0.f}, {IColor(255, 52, 199, 89), 1.f}}), 2.5f);

  // Draw region divider lines
  IColor dividerColor(100, 180, 180, 180);
  const float div1X = mPlotBounds.L + 0.3f * mPlotBounds.W();
  const float div2X = mPlotBounds.L + 0.7f * mPlotBounds.W();
  g.DrawLine(dividerColor, div1X, mPlotBounds.T + 5.f, div1X, mPlotBounds.B - 5.f, nullptr, 1.f);
  g.DrawLine(dividerColor, div2X, mPlotBounds.T + 5.f, div2X, mPlotBounds.B - 5.f, nullptr, 1.f);

  // Draw region marker labels
  IText labelFont(11, EAlign::Center, IColor(255, 100, 100, 100));

  const float labelY = mPlotBounds.B + 15.f;
  g.DrawText(labelFont, "ATTACK", mPlotBounds.L + 0.15f * mPlotBounds.W(), labelY);
  g.DrawText(labelFont, "SUSTAIN", mPlotBounds.L + 0.5f * mPlotBounds.W(), labelY);
  g.DrawText(labelFont, "OUTPUT", mPlotBounds.L + 0.85f * mPlotBounds.W(), labelY);
}
