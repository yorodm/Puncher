#include "EnvelopeDisplayControl.h"
#include "Puncher.h"
#include <vector>

EnvelopeDisplayControl::EnvelopeDisplayControl(const IRECT& bounds, int attackParam, int sustainParam, int outputParam)
: IControl(bounds, {attackParam, sustainParam, outputParam})
, mAttackParam(attackParam), mSustainParam(sustainParam), mOutputParam(outputParam)
{
}

void EnvelopeDisplayControl::OnResize()
{
  if(mRECT.Empty()) return;
  const float pad = 10.f;
  const float topPad = 25.f; // space for labels
  mPlotBounds = mRECT.GetPadded(-pad, -topPad, -pad, -pad);
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

  // Calculate envelope curve points
  const int numPoints = 100;
  std::vector<float> plotYs(numPoints);

  for (int i = 0; i < numPoints; i++)
  {
    const float t = static_cast<float>(i) / (numPoints - 1);
    const float x = mPlotBounds.L + t * mPlotBounds.W();

    float y;
    if (t < 0.3f)
    {
      const float attackT = t / 0.3f;
      const float peak = 0.2f + attackNorm * 0.8f;
      y = attackT * peak;
    }
    else if (t < 0.7f)
    {
      const float peak = 0.2f + attackNorm * 0.8f;
      const float sustainLevel = sustainNorm * peak;
      y = sustainLevel;
    }
    else
    {
      const float sustainLevel = sustainNorm * (0.2f + attackNorm * 0.8f);
      const float decayT = (t - 0.7f) / 0.3f;
      y = sustainLevel * (1.0f - decayT * 0.5f);
    }

    plotYs[i] = mPlotBounds.B - y * mPlotBounds.H();
  }

  // Build and fill the area path (curve + bottom edge)
  g.PathMoveTo(mPlotBounds.L, mPlotBounds.B);
  for (int i = 0; i < numPoints; i++)
  {
    const float x = mPlotBounds.L + static_cast<float>(i) / (numPoints - 1) * mPlotBounds.W();
    g.PathLineTo(x, plotYs[i]);
  }
  g.PathLineTo(mPlotBounds.R, mPlotBounds.B);
  g.PathClose();

  g.PathFill(IPattern::CreateLinearGradient(mPlotBounds, EDirection::Vertical,
    {{IColor(80, 66, 135, 245), 0.f}, {IColor(80, 52, 199, 89), 1.f}}));

  // Draw curve stroke
  g.PathMoveTo(mPlotBounds.L, plotYs[0]);
  for (int i = 1; i < numPoints; i++)
  {
    const float x = mPlotBounds.L + static_cast<float>(i) / (numPoints - 1) * mPlotBounds.W();
    g.PathLineTo(x, plotYs[i]);
  }
  g.PathStroke(IPattern::CreateLinearGradient(mPlotBounds, EDirection::Vertical,
    {{IColor(255, 66, 135, 245), 0.f}, {IColor(255, 52, 199, 89), 1.f}}), 2.5f);

  // Draw region divider lines
  IColor dividerColor(100, 180, 180, 180);
  const float div1X = mPlotBounds.L + 0.3f * mPlotBounds.W();
  const float div2X = mPlotBounds.L + 0.7f * mPlotBounds.W();
  g.DrawLine(dividerColor, div1X, mPlotBounds.T + 5.f, div1X, mPlotBounds.B - 5.f, nullptr, 1.f);
  g.DrawLine(dividerColor, div2X, mPlotBounds.T + 5.f, div2X, mPlotBounds.B - 5.f, nullptr, 1.f);

  // Draw region marker labels at the top
  IText labelFont(11, EAlign::Center, IColor(255, 100, 100, 100));
  const float labelY = mPlotBounds.T - 12.f;
  g.DrawText(labelFont, "ATTACK", mPlotBounds.L + 0.15f * mPlotBounds.W(), labelY);
  g.DrawText(labelFont, "SUSTAIN", mPlotBounds.L + 0.5f * mPlotBounds.W(), labelY);
}
