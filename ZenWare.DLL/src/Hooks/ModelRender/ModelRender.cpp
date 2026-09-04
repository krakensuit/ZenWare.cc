#include "ModelRender.h"
#include "../../Util/Logger/Logger.h"

#include "../../Features/Chams/Chams.h"

using namespace Hooks;

void __fastcall ModelRender::ForcedMaterialOverride::Detour(void* ecx, void* edx, IMaterial* newMaterial, OverrideType_t nOverrideType)
{
	ZTRACE_FIRST("ModelRender::ForcedMaterialOverride");
	Table.Original<FN>(Index)(ecx, edx, newMaterial, nOverrideType);
}

void __fastcall ModelRender::DrawModelExecute::Detour(void* ecx, void* edx, const DrawModelState_t& state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	ZTRACE_FIRST("ModelRender::DrawModelExecute");
	const bool bChams = F::Chams.OnDrawModel(pInfo);

	Table.Original<FN>(Index)(ecx, edx, state, pInfo, pCustomBoneToWorld);

	if (bChams)
		F::Chams.OnDrawModelEnd();
}

void ModelRender::Init()
{
	XASSERT(Table.Init(I::ModelRender) == false);
	XASSERT(Table.Hook(&ForcedMaterialOverride::Detour, ForcedMaterialOverride::Index) == false);
	XASSERT(Table.Hook(&DrawModelExecute::Detour, DrawModelExecute::Index) == false);
}