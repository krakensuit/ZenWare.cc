#include "Visuals.h"

#include "../Vars.h"

static const Color CLR_TEXT_HINT(140, 160, 152, 255);

void CFeatures_Visuals::UpdateThirdPerson()
{
	static bool s_bWasOn = false;
	static int s_nLastDist = -1;
	const bool bWant = Vars::Visuals::bThirdPerson && I::EngineClient && I::EngineClient->IsInGame();
	const int nDist = U::Math.Clamp(Vars::Visuals::nThirdPersonDist, 30, 200);

	if (bWant)
	{
		// Дистанцию досылаем и на ходу, а не только в момент включения.
		if (!s_bWasOn || nDist != s_nLastDist)
		{
			char szCmd[96] = { };
			//локальный сервер: включаем камеру и дистанцию
			sprintf_s(szCmd, "sv_cheats 1; cam_idealdist %d; cam_idealpitch 0; thirdperson", nDist);
			I::EngineClient->ClientCmd_Unrestricted(szCmd);
			s_nLastDist = nDist;
		}
		s_bWasOn = true;
		return;
	}

	if (s_bWasOn)
	{
		s_bWasOn = false;
		s_nLastDist = -1;
		if (I::EngineClient)
			I::EngineClient->ClientCmd_Unrestricted("firstperson");
	}
}

void CFeatures_Visuals::DrawCrosshair()
{
	if (!Vars::Visuals::bCrosshair || !G::Draw.m_nScreenW)
		return;

	const int nCX = G::Draw.m_nScreenW / 2;
	const int nCY = G::Draw.m_nScreenH / 2;
	const int nS = U::Math.Clamp(Vars::Visuals::nCrosshairSize, 2, 40);
	const Color& clr = Vars::Visuals::clrCrosshair;

 //Gap breathes with movement speed.
 float flSpeed = 0.0f;
 C_TerrorPlayer* pLocal = nullptr;
 {
  const int nLocalIdx = I::EngineClient->GetLocalPlayer();
  if (nLocalIdx >= 0)
  {
   IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
   if (pEnt) pLocal = pEnt->As<C_TerrorPlayer*>();
  }
 }

 if (pLocal)
  flSpeed = pLocal->m_vecVelocity().Lenght2D();

	const int nGap = 4 + U::Math.Clamp((int)(flSpeed / 50.0f), 0, 12);
	constexpr int nThick = 2;

	G::Draw.Rect(nCX - nGap - nS, nCY - (nThick / 2), nS, nThick, clr);
	G::Draw.Rect(nCX + nGap, nCY - (nThick / 2), nS, nThick, clr);
	G::Draw.Rect(nCX - (nThick / 2), nCY - nGap - nS, nThick, nS, clr);
	G::Draw.Rect(nCX - (nThick / 2), nCY + nGap, nThick, nS, clr);
	G::Draw.Rect(nCX - 1, nCY - 1, 2, 2, clr);
}

void CFeatures_Visuals::DrawOverlay()
{
	if (!I::EngineClient->IsInGame() || !I::GlobalVars)
		return;

 C_TerrorPlayer* pLocal = nullptr;
 {
  const int nLocalIdx = I::EngineClient->GetLocalPlayer();
  if (nLocalIdx >= 0)
  {
   IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
   if (pEnt) pLocal = pEnt->As<C_TerrorPlayer*>();
  }
 }

 if (!pLocal)
  return;

	static float s_flFpsAvg = 0.0f;
	const float flFps = (I::GlobalVars->frametime > 0.0f) ? (1.0f / I::GlobalVars->frametime) : 0.0f;
	s_flFpsAvg = (s_flFpsAvg == 0.0f) ? flFps : (s_flFpsAvg * 0.95f + flFps * 0.05f);

	const Vector vPos = pLocal->m_vecOrigin();
	const float flSpeed = pLocal->m_vecVelocity().Lenght2D();

	if (Vars::Visuals::bOverlay)
	{
		G::Draw.String(EFonts::MENU_CONSOLAS, 8, G::Draw.m_nScreenH - 34,
			CLR_TEXT_HINT, TXT_DEFAULT, "fps %4.0f | pos %.0f %.0f %.0f | hp %i",
			s_flFpsAvg, vPos.x, vPos.y, vPos.z, pLocal->GetHealth());
	}

	// Speed HUD - movement feature
	if (Vars::BunnyHop::bSpeedHUD)
	{
		Color clrSpd = { 255, 255, 255, 255 };
		if (flSpeed > 300) clrSpd = { 0, 255, 171, 255 };
		if (flSpeed > 400) clrSpd = { 255, 220, 0, 255 };
		G::Draw.String(EFonts::MENU_CONSOLAS, 8, G::Draw.m_nScreenH - 54,
			clrSpd, TXT_DEFAULT, "speed %.0f u/s", flSpeed);
	}
}
