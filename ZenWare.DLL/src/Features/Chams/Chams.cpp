#include "Chams.h"

#include "../Vars.h"

void CFeatures_Chams::LazyInit()
{
	if (m_bInitialized)
		return;

	m_bInitialized = true;

	constexpr const char* szVMT =
		R"("UnlitGeneric"
{
	"$basetexture"	"vgui/white_additive"
	"$ignorez"	"%i"
	"$model"	"1"
	"$flat"	"1"
	"$nocull"	"1"
	"$selfillum"	"1"
	"$halflambert"	"1"
}
)";

	char szBuf[512] = { };

	sprintf_s(szBuf, sizeof(szBuf), szVMT, 0);
	m_pEnemy = G::Util.CreateMaterial(szBuf);

	sprintf_s(szBuf, sizeof(szBuf), szVMT, 0);
	m_pAlly = G::Util.CreateMaterial(szBuf);

	sprintf_s(szBuf, sizeof(szBuf), szVMT, 0);
	m_pTank = G::Util.CreateMaterial(szBuf);
}

bool CFeatures_Chams::OnDrawModel(const ModelRenderInfo_t& pInfo)
{
	ApplyPalette();

	if (!Vars::Chams::bEnabled || !I::EngineClient->IsInGame())
		return false;

	if (!(pInfo.flags & STUDIO_RENDER) || (pInfo.flags & STUDIO_SHADOWDEPTHTEXTURE))
		return false;

	LazyInit();

	IClientEntity* pIClient = I::ClientEntityList->GetClientEntity(pInfo.entity_index);

	if (!pIClient)
		return false;

	C_TerrorPlayer* pPlayer = pIClient->As<C_TerrorPlayer*>();

	if (!pPlayer)
		return false;

	const int nLocalIndex = I::EngineClient->GetLocalPlayer();

	if (pInfo.entity_index == nLocalIndex)
		return false;

	C_TerrorPlayer* pLocal = I::ClientEntityList->GetClientEntity(nLocalIndex)->As<C_TerrorPlayer*>();

	//Shared target filter (no visibility check for rendering).
	if (!G::Util.IsValidTarget(pLocal, pPlayer, false))
		return false;

	//Enemy/ally is resolved relative to the local player's team.
	const bool bIsEnemy = (pLocal && pPlayer->GetTeamNumber() != pLocal->GetTeamNumber());

	IMaterial* pMaterial = nullptr;
	Color clr = Vars::Chams::clrAlly;

	if (bIsEnemy)
	{
		if (pPlayer->m_zombieClass() == CLASS_TANK)
		{
			pMaterial = m_pTank;
			clr = Vars::Chams::clrTank;
		}
		else
		{
			pMaterial = m_pEnemy;
			clr = Vars::Chams::clrEnemy;
		}
	}
	else
	{
		pMaterial = m_pAlly;
	}

	if (!pMaterial || IsErrorMaterial(pMaterial))
		return false;

	pMaterial->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, Vars::Chams::bThroughWalls);
	pMaterial->ColorModulate(clr.r() / 255.0f, clr.g() / 255.0f, clr.b() / 255.0f);
	pMaterial->AlphaModulate(clr.a() / 255.0f);

	I::ModelRender->ForcedMaterialOverride(pMaterial, OVERRIDE_NORMAL);

	return true;
}

void CFeatures_Chams::OnDrawModelEnd()
{
	I::ModelRender->ForcedMaterialOverride(nullptr, OVERRIDE_NORMAL);
}

void CFeatures_Chams::ApplyPalette()
{
	static const Color sc_palettes[][3] = {
		{ { 150,  15,  15, 255 }, {  15, 150, 150, 255 }, { 150, 100,  15, 255 } }, //0 classic
		{   { 0, 200, 130, 255 }, { 170, 170, 190, 255 }, { 255, 210,   0, 255 } }, //1 mint
		{ { 170,  80, 255, 255 }, {  90, 255, 120, 255 }, { 255, 120, 220, 255 } }, //2 neon
		{ { 240, 240, 240, 255 }, {  60,  60,  65, 255 }, { 255, 140,   0, 255 } }, //3 mono
		{ { 255, 190,   0, 255 }, {   0, 200, 255, 255 }, { 255,  90,   0, 255 } }, //4 gold/cyan
	};
	constexpr int nCount = 5;

	Vars::Chams::nPalette = U::Math.Clamp(Vars::Chams::nPalette, 0, nCount - 1);

	const auto& p = sc_palettes[Vars::Chams::nPalette];
	Vars::Chams::clrEnemy = p[0];
	Vars::Chams::clrAlly = p[1];
	Vars::Chams::clrTank = p[2];
}
