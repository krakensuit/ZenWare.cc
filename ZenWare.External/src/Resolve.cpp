#include "Resolve.h"
#include "Offsets.h"
#include <cstring>
#include <cmath>

namespace
{
#pragma pack(push, 1)
	struct DosHdr { uint16_t e_magic; uint16_t e_rest[29]; int32_t e_lfanew; };
	struct CoffHdr { uint32_t sig; uint16_t mach, nsec; uint32_t ts, sym, nsym; uint16_t optsz, chars; };
	struct Opt32 { uint16_t magic; uint8_t rest[26]; uint32_t imageBase; };
	struct SecHdr { char name[8]; uint32_t vsize, vaddr, rawsize, rawptr, reloc, line, nreloc, nline; uint32_t chars; };
#pragma pack(pop)

	// Диапазон секции загруженного модуля: [base+rva, base+rva+vsize).
	bool SectionRange(const uint8_t* img, size_t len, const char* name, uintptr_t base, uintptr_t& outBase, uint32_t& outSize)
	{
		if (len < sizeof(DosHdr))
			return false;
		const DosHdr* dos = (const DosHdr*)img;
		if (dos->e_magic != 0x5A4D || dos->e_lfanew <= 0)
			return false;
		size_t nt = (size_t)dos->e_lfanew;
		if (nt + sizeof(CoffHdr) + sizeof(Opt32) > len)
			return false;
		const CoffHdr* coff = (const CoffHdr*)(img + nt);
		if (coff->sig != 0x4550 || coff->mach != 0x14C)
			return false;
		const Opt32* opt = (const Opt32*)(img + nt + sizeof(CoffHdr));
		if (opt->magic != 0x10B)
			return false;
		size_t secs = nt + sizeof(CoffHdr) + coff->optsz;
		if (secs + (size_t)coff->nsec * sizeof(SecHdr) > len)
			return false;
		for (int i = 0; i < coff->nsec; i++)
		{
			const SecHdr* s = (const SecHdr*)(img + secs + (size_t)i * sizeof(SecHdr));
			if (memcmp(s->name, name, strlen(name)) == 0 && s->vsize > 0)
			{
				outBase = base + s->vaddr;
				outSize = s->vsize;
				return true;
			}
		}
		return false;
	}

	uint32_t ImageBase(const uint8_t* img, size_t len)
	{
		if (len < sizeof(DosHdr))
			return 0;
		const DosHdr* dos = (const DosHdr*)img;
		if (dos->e_magic != 0x5A4D || dos->e_lfanew <= 0)
			return 0;
		size_t nt = (size_t)dos->e_lfanew;
		if (nt + sizeof(CoffHdr) + sizeof(Opt32) > len)
			return 0;
		return ((const Opt32*)(img + nt + sizeof(CoffHdr)))->imageBase;
	}

	// Сканер по буферу (копия Memory::Scan, но с отчетом всех совпадений).
	int FindAll(const uint8_t* data, size_t len, const std::vector<uint8_t>& b, const std::vector<bool>& m, int* out, int cap)
	{
		int n = 0;
		for (size_t i = 0; i + b.size() <= len && n < cap; i++)
		{
			bool hit = true;
			for (size_t j = 0; j < b.size(); j++)
				if (m[j] && data[i + j] != b[j]) { hit = false; break; }
			if (hit)
				out[n++] = (int)i;
		}
		return n;
	}

	bool ParseSig(const char* pat, std::vector<uint8_t>& b, std::vector<bool>& m)
	{
		char tok[8] = { };
		while (*pat)
		{
			while (*pat == ' ') pat++;
			if (!*pat) break;
			int k = 0;
			while (*pat && *pat != ' ' && k < 7) tok[k++] = *pat++;
			tok[k] = 0;
			if (!strcmp(tok, "?") || !strcmp(tok, "??")) { b.push_back(0); m.push_back(false); }
			else { b.push_back((uint8_t)strtoul(tok, nullptr, 16)); m.push_back(true); }
		}
		return !b.empty();
	}

	bool Finite16(const float* f)
	{
		for (int i = 0; i < 16; i++)
			if (!isfinite(f[i]) || fabsf(f[i]) > 100000.0f)
				return false;
		return true;
	}

	float Det4(const float* m)
	{
		// Определитель 4x4 через разложение (вырожденная матрица = мусор).
		float a2323 = m[10] * m[15] - m[11] * m[14];
		float a1323 = m[9] * m[15] - m[11] * m[13];
		float a1223 = m[9] * m[14] - m[10] * m[13];
		float a0323 = m[8] * m[15] - m[11] * m[12];
		float a0223 = m[8] * m[14] - m[10] * m[12];
		float a0123 = m[8] * m[13] - m[9] * m[12];
		return m[0] * (m[5] * a2323 - m[6] * a1323 + m[7] * a1223)
			- m[1] * (m[4] * a2323 - m[6] * a0323 + m[7] * a0223)
			+ m[2] * (m[4] * a1323 - m[5] * a0323 + m[7] * a0123)
			- m[3] * (m[4] * a1223 - m[5] * a0223 + m[6] * a0123);
	}

	struct EntCheck { int team, hp; uint8_t life; };

	bool ReadEnt(const Memory& mem, uintptr_t ent, EntCheck& e)
	{
		if (!mem.Read(ent + Off::offTeam, e.team) || (e.team != 2 && e.team != 3))
			return false;
		if (!mem.Read(ent + Off::offHealth, e.hp) || e.hp <= 0 || e.hp > 20000)
			return false;
		if (!mem.Read(ent + Off::offLifeState, e.life) || e.life != 0)
			return false;
		return true;
	}
}

bool ResolveOffsets(const Memory& mem, uintptr_t client, uint32_t clientSize,
	uintptr_t engine, uint32_t engineSize, Resolved_t& out)
{
	out = Resolved_t();
	out.local = Off::dwLocalPlayer; strcpy_s(out.srcLocal, "hard");
	out.list = Off::dwEntityList; strcpy_s(out.srcList, "hard");
	out.mat = Off::dwViewMatrix; strcpy_s(out.srcMat, "hard");

	if (!client || !engine || clientSize < 0x100000 || engineSize < 0x100000)
		return false;

	std::vector<uint8_t> cli(clientSize);
	if (!mem.ReadRaw(client, cli.data(), cli.size()))
		return false;
	const uint32_t cliBase = ImageBase(cli.data(), cli.size());

	uintptr_t anchor = 0; // валидный указатель локального игрока

	// --- 1. LocalPlayer по сигнатуре (уникальна, проверено на нашем билде) ---
	{
		std::vector<uint8_t> b; std::vector<bool> m;
		ParseSig("8B 0D ? ? ? ? 85 C9 74 ? 8B 01 8B 50 08 FF D2 8B 00 89 86 84 16 00 00", b, m);
		int hits[4] = { };
		if (FindAll(cli.data(), cli.size(), b, m, hits, 4) == 1 && cliBase)
		{
			uint32_t disp;
			memcpy(&disp, cli.data() + hits[0] + 2, 4);
			if (disp >= cliBase && disp - cliBase < clientSize)
			{
				uintptr_t ent = 0;
				uintptr_t addr = client + (disp - cliBase);
				EntCheck e;
				if (mem.Read(addr, ent) && ent && ReadEnt(mem, ent, e))
				{
					// origin тоже валидируем, как Snapshot
					float org[3] = { };
					bool okOrg = mem.ReadRaw(ent + Off::offOrigin, org, sizeof(org));
					bool fin = okOrg;
					for (int i = 0; i < 3 && fin; i++)
						fin = isfinite(org[i]) && fabsf(org[i]) < 20000.0f;
					if (fin)
					{
						out.local = disp - cliBase;
						strcpy_s(out.srcLocal, "sig");
						anchor = ent;
					}
				}
			}
		}
	}

	std::vector<uint8_t> eng;
	uintptr_t engData = 0; uint32_t engDataSize = 0;
	bool haveEng = false;
	{
		eng.assign(engineSize, 0);
		if (mem.ReadRaw(engine, eng.data(), eng.size()))
		{
			haveEng = true;
			SectionRange(eng.data(), eng.size(), ".data", engine, engData, engDataSize);
		}
	}

	// --- 2. ViewMatrix: перебор thunk'ов B9 imm32 / E9 rel32, imm обязан лежать в .data ---
	if (haveEng && engDataSize > 0)
	{
		std::vector<uint8_t> b; std::vector<bool> m;
		ParseSig("B9 ? ? ? ? E9", b, m);
		int hits[2048] = { };
		int n = FindAll(eng.data(), eng.size(), b, m, hits, 2048);
		int passed = 0;
		for (int k = 0; k < n; k++)
		{
			uint32_t imm;
			memcpy(&imm, eng.data() + hits[k] + 1, 4);
			uintptr_t obj = (uintptr_t)imm; // в файле/памяти адрес уже rebased
			if (obj < engData || obj >= engData + engDataSize)
				continue;
			uintptr_t ptr = 0;
			if (!mem.Read(obj, ptr) || !ptr)
				continue;
			if (ptr >= engine && ptr < engine + engineSize)
				continue; // указатель в код/модуль — не куча рендера
			if (ptr >= client && ptr < client + clientSize)
				continue;
			float f[16] = { };
			if (!mem.ReadRaw(ptr + Off::dwViewMatrixInner, f, sizeof(f)) || !Finite16(f))
				continue;
			if (fabsf(Det4(f)) < 1e-6f)
				continue;
			passed++;
			if (!out.mat || !strcmp(out.srcMat, "hard"))
			{
				// первый прошедший — кандидат; дальше только считаем для диагностики
				out.mat = obj - engine;
				strcpy_s(out.srcMat, "sig");
			}
		}
		out.matCands = passed;
		if (!passed)
		{
			out.mat = Off::dwViewMatrix;
			strcpy_s(out.srcMat, "hard");
		}
	}

	// --- 3. EntityList: якорный скан .data по значению anchor ---
	if (anchor)
	{
		uintptr_t cliData = 0; uint32_t cliDataSize = 0;
		if (SectionRange(cli.data(), cli.size(), ".data", client, cliData, cliDataSize) && cliDataSize > 0x10000)
		{
			size_t dOff = (size_t)(cliData - client), dEnd = dOff + cliDataSize;
			if (dEnd <= cli.size())
			{
				int best = -1, bestScore = 0;
				uintptr_t bestBase = 0;
				// слоты: локальный игрок обычно в первых десятках
				for (size_t o = dOff; o + 4 <= dEnd; o += 4)
				{
					uint32_t v;
					memcpy(&v, cli.data() + o, 4);
					if (v != (uint32_t)(anchor & 0xFFFFFFFFu))
						continue;
					for (int i = 0; i < 96; i++)
					{
						uintptr_t base = cliData + (o - dOff) - (uintptr_t)i * 0x10;
						if (base < cliData)
							break;
						int score = 0;
						for (int j = 0; j < 24; j++)
						{
							uintptr_t ent = 0;
							if (!mem.Read(base + (uintptr_t)j * 0x10, ent) || !ent)
								continue;
							EntCheck e;
							if (ReadEnt(mem, ent, e))
								score++;
						}
						if (score > bestScore)
						{
							bestScore = score;
							best = i;
							bestBase = base;
							if (score >= 12)
								break;
						}
					}
					if (bestScore >= 12)
						break;
				}
				if (best >= 0 && bestScore >= 4)
				{
					out.list = bestBase - client;
					strcpy_s(out.srcList, "anchor");
				}
			}
		}
	}

	return true;
}
