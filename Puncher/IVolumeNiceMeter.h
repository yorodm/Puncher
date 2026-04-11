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
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;

private:
  void DrawChannel(IGraphics& g, const IRECT& r, int ch);
  float AmpToDB(float amp) const;
  float DBToNorm(float db) const;

private:
  // dB values for display (avg for bar fill, peak for indicator)
  std::array<float, 2> mAvgDB   = {-60.f, -60.f};
  std::array<float, 2> mPeakDB  = {-60.f, -60.f};

  // Clip flags (persist until reset)
  bool mClip[2] = {false, false};

  // Settings (dBFS: 0 = full scale / clipping)
  float mMinDB = -60.f;
  float mMaxDB = 0.f;
};
