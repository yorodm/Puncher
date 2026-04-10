#pragma once

#include "IControl.h"

using namespace iplug;
using namespace igraphics;

class EnvelopeDisplayControl : public IControl
{
public:
  EnvelopeDisplayControl(const IRECT& bounds, int attackParam, int sustainParam, int outputParam);
  void Draw(IGraphics& g) override;
  void OnResize() override;

private:
  int mAttackParam;
  int mSustainParam;
  int mOutputParam;
  IRECT mPlotBounds;
};
