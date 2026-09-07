#include "Hitmarker.h"
#include "../Vars.h"
#include "../../SDK/DrawManager/DrawManager.h"

#include <thread>

namespace
{
	// Best-effort опознание shootable-врага, зеркалит классификацию Aimbot:
	// игроки чужой команды, особые, обычные + ведьма. Возвращает HP и якорь.
	bool GetEnemyHp(IClientEntity* pEntity, C_TerrorPlayer* pLocal, int& nHpOut, Vector& vAnchorOut)
	{
		ClientClass* pCC = pEntity->GetClientClass();
		if (!pCC)
			return false;

		const int nID = pCC->m_ClassID;

		// Выжившие/боты чужой команды (+танк за игроков).
		if (nID == CTerrorPlayer || nID == SurvivorBot || nID == Tank)
		{
			C_TerrorPlayer* pPlayer = pEntity->As<C_TerrorPlayer*>();
			if (!G::Util.IsValidTarget(pLocal, pPlayer, false))
				return false;
			nHpOut = pPlayer->GetHealth();
			vAnchorOut = pPlayer->m_vecOrigin() + Vector(0.0f, 0.0f, pPlayer->m_vecMaxs().z * 0.7f);
			return nHpOut >= 0;
		}

		// Особые заражённые.
		if (nID == Hunter || nID == Smoker || nID == Jockey || nID == Spitter || nID == Charger)
		{
			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
			if (!pEnt || !pPl || pPl->m_lifeState() != 0)
				return false;
			const int nTeam = pEnt->m_iTeamNum();
			if (nTeam == pLocal->GetTeamNumber())
				return false;
			nHpOut = pEnt->GetHealth();
			vAnchorOut = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.7f);
			return nHpOut > 0;
		}

		// Обычные + ведьма.
		if (nID == Infected || nID == Witch)
		{
			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			C_Infected* pInf = pEntity->As<C_Infected*>();
			if (!pEnt || !pInf)
				return false;
			if (!G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
				return false;
			nHpOut = pEnt->GetHealth();
			vAnchorOut = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.7f);
			return nHpOut > 0;
		}

		return false;
	}

	bool IsVisibleTo(C_TerrorPlayer* pLocal, const Vector& vEye, const Vector& vPoint)
	{
		trace_t tr;
		CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));
		G::Util.Trace(vEye, vPoint, MASK_SHOT, &filter, &tr);
		return !tr.DidHit();
	}
}

void CFeatures_Hitmarker::PlayHit(bool bKill)
{
	if (!Vars::Hitmarker::bSound)
		return;

	// Антиспам: дробовики/ очередь бьют каждый кадр.
	const unsigned long long ullNow = GetTickCount64();
	if (ullNow - m_ullLastSnd < 40)
		return;
	m_ullLastSnd = ullNow;

	const int nPitch = U::Math.Clamp(Vars::Hitmarker::nPitch, 200, 2000);
	const int nFreq = bKill ? (nPitch * 3 / 2) : nPitch;
	// Beep синхронный — уводим в фон, кадр не ждёт.
	std::thread([nFreq, bKill]() { Beep(nFreq, bKill ? 120 : 60); }).detach();
}

void CFeatures_Hitmarker::OnTick()
{
	if (!Vars::Hitmarker::bEnabled || !I::EngineClient || !I::EngineClient->IsInGame() || !I::ClientEntityList || !I::GlobalVars)
	{
		if (!m_mHp.empty() || !m_aNums.empty()) { m_mHp.clear(); m_aNums.clear(); }
		return;
	}

	C_TerrorPlayer* pLocal = nullptr;
	{
		const int nLocalIdx = I::EngineClient->GetLocalPlayer();
		if (nLocalIdx > 0)
		{
			IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
			if (pEnt) pLocal = pEnt->As<C_TerrorPlayer*>();
		}
	}
	if (!pLocal || pLocal->deadflag() || pLocal->m_lifeState() != 0)
		return;

	const Vector vEye = G::Util.GetEyePosition(pLocal);

	// Зачёт только пока жмём огонь (мышь или клавиша аима): иначе это урон союзников.
	const bool bFiring = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0
		|| (Vars::Aimbot::nKey && (GetAsyncKeyState(Vars::Aimbot::nKey) & 0x8000) != 0);

	const float flNow = I::GlobalVars->curtime;
	std::map<int, int> mSeen;

	const int nMax = I::ClientEntityList->GetMaxEntities();
	for (int n = 1; n <= nMax; n++)
	{
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
		if (!pEntity || pEntity->IsDormant())
			continue;

		int nHp = 0;
		Vector vAnchor;
		if (!GetEnemyHp(pEntity, pLocal, nHp, vAnchor))
			continue;

		mSeen[n] = nHp;

		auto it = m_mHp.find(n);
		const int nPrev = (it == m_mHp.end()) ? nHp : it->second;
		const int nDmg = nPrev - nHp;

		// Скачок вверх = хил/респавн, огромный скачок вниз = телепорт сущности.
		if (nDmg <= 0 || nDmg > 2000)
			continue;
		if (!bFiring)
			continue;
		if (!IsVisibleTo(pLocal, vEye, vAnchor))
			continue;

		const bool bKill = (nHp <= 0);
		PlayHit(bKill);

		if (Vars::Hitmarker::bNumbers && m_aNums.size() < 12)
		{
			HitNum_t m;
			m.vWorld = vAnchor;
			m.nDamage = nDmg;
			m.flT = flNow;
			m.bKill = bKill;
			m_aNums.push_back(m);
		}
	}

	m_mHp.swap(mSeen);
}

void CFeatures_Hitmarker::Draw()
{
	if (m_aNums.empty() || !G::Draw.m_nScreenW || !I::GlobalVars)
		return;

	const float flNow = I::GlobalVars->curtime;
	const float flDur = U::Math.Clamp(Vars::Hitmarker::nDurationMs, 400, 3000) / 1000.0f;

	for (int i = (int)m_aNums.size() - 1; i >= 0; --i)
	{
		HitNum_t& m = m_aNums[i];
		const float flAge = flNow - m.flT;
		if (flAge > flDur) { m_aNums.erase(m_aNums.begin() + i); continue; }
		if (flAge < 0.0f)
			continue;

		Vector vS;
		if (!G::Util.W2S(m.vWorld, vS))
			continue;

		const int nA = (int)(255.0f * (1.0f - flAge / flDur));
		const int nY = (int)vS.y - (int)(flAge * 46.0f);
		char sz[32] = { };
		sprintf_s(sz, m.bKill ? "KILL -%d" : "-%d", m.nDamage);
		const Color clr = m.bKill ? Color(255, 70, 70, nA) : Color(0, 255, 171, nA);
		G::Draw.String(EFonts::ESP_NAME, (int)vS.x, nY, clr, TXT_CENTERXY, "%s", sz);
	}
}
