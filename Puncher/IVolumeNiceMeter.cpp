#include "IVolumeNiceMeter.h"
#include "IGraphics.h"
#include <cmath>
#include <algorithm>
#include <chrono>

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
  if (dataSize == sizeof(float) * 2)
  {
    const float* levels = (const float*) pData;
    double now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0;

    for (int ch = 0; ch < 2; ch++)
    {
      float amp = std::max(0.f, levels[ch]);
      float db = AmpToDB(amp);

      mLevels[ch] = db;

      // Clip detection
      mClip[ch] = amp >= 1.f;

      // Peak logic
      if (db > mPeaks[ch])
      {
        mPeaks[ch] = db;
        mLastUpdateTime[ch] = now;
      }
      else
      {
        double dt = now - mLastUpdateTime[ch];

        if (dt > mPeakHoldTime)
        {
          mPeaks[ch] -= mPeakDecay * dt;
          mPeaks[ch] = std::max(mPeaks[ch], mMinDB);
          mLastUpdateTime[ch] = now;
        }
      }
    }

    SetDirty(false);
  }
}

void IVolumeNiceMeter::Draw(IGraphics& g)
{
  IRECT bounds = mRECT;
  float half = bounds.W() * 0.5f;

  IRECT left  = bounds.GetFromLeft(half).GetPadded(-4);
  IRECT right = bounds.GetFromRight(half).GetPadded(-4);

  DrawChannel(g, left, 0);
  DrawChannel(g, right, 1);

  // Draw dB markers
  for (float db : kMarkers)
  {
    float norm = DBToNorm(db);
    float y = bounds.B - norm * bounds.H();

    g.DrawLine(IColor(120, 255, 255, 255),
               bounds.L, y,
               bounds.R, y,
               nullptr, 1.0f);

    g.DrawText(IText(10),
               std::to_string((int) db).c_str(),
               IRECT(bounds.L, y - 6, bounds.L + 30, y + 6));
  }
}

void IVolumeNiceMeter::DrawChannel(IGraphics& g, const IRECT& r, int ch)
{
  float levelDB = std::clamp(mLevels[ch], mMinDB, mMaxDB);
  float peakDB  = std::clamp(mPeaks[ch],  mMinDB, mMaxDB);

  float normLevel = DBToNorm(levelDB);
  float normPeak  = DBToNorm(peakDB);

  // 3-color gradient (green → yellow → red) via IPattern
  IPattern grad = IPattern::CreateLinearGradient(
    r,
    EDirection::Vertical,
    {
      {IColor(255, 255, 0, 0), 1.0f},  // red at the top
      {IColor(255, 255, 255, 0), 0.7f},   // yellow
      {IColor(255, 0, 255, 0), 0.0f},     // green at bottom
    }
  );

  // Fill the bar up to normLevel using PathFill for gradient support
  IRECT fillRect = r.FracRectVertical(normLevel, false); // false = fill from bottom
  if (!fillRect.Empty())
  {
    g.PathMoveTo(fillRect.L, fillRect.B);
    g.PathLineTo(fillRect.R, fillRect.B);
    g.PathLineTo(fillRect.R, fillRect.T);
    g.PathLineTo(fillRect.L, fillRect.T);
    g.PathClose();
    g.PathFill(grad);
  }

  // Peak line
  float peakY = r.B - normPeak * r.H();

  g.DrawLine(IColor(255, 255, 255, 255),
             r.L, peakY,
             r.R, peakY,
             nullptr, 2.0f);

  // Clip indicator (top bar)
  if (mClip[ch])
  {
    IRECT clipRect = r.GetFromTop(5);
    g.FillRect(IColor(255, 255, 0, 0), clipRect);
  }
}
