#pragma once

#include "../../SDK/SDK.h"

class CFeatures_VisualRecoil
{
public:
	void FrameStageNotify(ClientFrameStage_t curStage);
};

namespace F { inline CFeatures_VisualRecoil VisualRecoil; }
