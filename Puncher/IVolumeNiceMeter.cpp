#include "IVolumeNiceMeter.h"
#include "IGraphics.h"
#include "ISender.h"
#include <cmath>
#include <algorithm>

IVolumeNiceMeter::IVolumeNiceMeter(const IRECT& bounds)
: IControl(bounds)
{}

float IVolumeNiceMeter::AmpToDB(float amp) const
{
  return 20.f * std::log10(std::max(amp, 1e-6f));
}

float IVolumeNiceMeter::DBToNorm(float db) const
{
  return (db - mMinDB) / (mMaxDB - mMinDB);
}

void IVolumeNiceMeter::OnMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
  if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
  {
    IByteStream stream(pData, dataSize);
    int pos = 0;
    ISenderData<2, std::pair<float, float>> d;
    stream.Get(&d, pos);

    for (int c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
    {
      float peakAmp = std::get<0>(d.vals[c]);
      float avgAmp  = std::get<1>(d.vals[c]);

      mPeakDB[c] = AmpToDB(peakAmp);
      mAvgDB[c]  = AmpToDB(avgAmp);

      // Clip detection — once set, stays true until reset
      if (!mClip[c])
        mClip[c] = peakAmp >= 1.f;
    }

    SetDirty(false);
  }
}

void IVolumeNiceMeter::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  mClip[0] = false;
  mClip[1] = false;
  mPeakDB[0] = mMinDB;
  mPeakDB[1] = mMinDB;
  mAvgDB[0] = mMinDB;
  mAvgDB[1] = mMinDB;
  SetDirty(false);
}

void IVolumeNiceMeter::Draw(IGraphics& g)
{
  IRECT bounds = mRECT;
  float half = bounds.W() * 0.5f;

  IRECT left  = bounds.GetFromLeft(half).GetPadded(-4);
  IRECT right = bounds.GetFromRight(half).GetPadded(-4);

  DrawChannel(g, left, 0);
  DrawChannel(g, right, 1);
}

void IVolumeNiceMeter::DrawChannel(IGraphics& g, const IRECT& r, int ch)
{
  float avgDB   = std::clamp(mAvgDB[ch], mMinDB, mMaxDB);
  float peakDB  = std::clamp(mPeakDB[ch], mMinDB, mMaxDB);

  float normAvg  = DBToNorm(avgDB);
  float normPeak = DBToNorm(peakDB);

  // Dark gray background (empty meter area)
  g.FillRect(IColor(255, 40, 40, 40), r);

  // Gradient fill for the volume level
  IRECT fillRect = r.FracRectVertical(normAvg, false);
  if (!fillRect.Empty())
  {
    IPattern grad = IPattern::CreateLinearGradient(
      r,
      EDirection::Vertical,
      {
        {IColor(255, 0, 255, 0), 0.0f},     // green at bottom (quiet)
        {IColor(255, 255, 255, 0), 0.7f},   // yellow
        {IColor(255, 255, 0, 0), 1.0f}      // red at top (loud)
      }
    );

    g.PathMoveTo(fillRect.L, fillRect.B);
    g.PathLineTo(fillRect.R, fillRect.B);
    g.PathLineTo(fillRect.R, fillRect.T);
    g.PathLineTo(fillRect.L, fillRect.T);
    g.PathClose();
    g.PathFill(grad);
  }

  // Always-visible peak indicator
  float peakY = r.B - normPeak * r.H();
  const IColor peakColor = mClip[ch] ? IColor(255, 255, 60, 60) : IColor(255, 255, 255, 255);

  IRECT peakRect = IRECT(r.L, peakY - 1.5f, r.R, peakY + 1.5f);
  g.FillRect(peakColor, peakRect);
}
