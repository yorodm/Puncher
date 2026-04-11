#pragma once

#include "IControl.h"
#include "IPlugStructs.h"
#include <array>

using namespace iplug;
using namespace igraphics;

class IVolumeNiceMeter : public IControl
{
public:
  IVolumeNiceMeter(const IRECT& bounds);

  void Draw(IGraphics& g) override;
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override;

private:
  void DrawChannel(IGraphics& g, const IRECT& r, int ch);
  float AmpToDB(float amp) const;
  float DBToNorm(float db) const;

private:
  std::array<float, 2> mLevels = {0.f, 0.f};
  std::array<float, 2> mPeaks  = {0.f, 0.f};

  std::array<double, 2> mLastUpdateTime = {0.0, 0.0};

  // Settings
  float mMinDB = -60.f;
  float mMaxDB = 0.f;

  float mPeakHoldTime = 0.8f;
  float mPeakDecay = 20.f; // dB per second

  bool mClip[2] = {false, false};

  // Marker values in dB
  static constexpr float kMarkers[] = {-60.f, -48.f, -36.f, -24.f, -12.f, -6.f, 0.f};
};
