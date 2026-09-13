#include "NetVarManager.h"

#include "../../SDK/L4D2/Includes/dt_recv.h"
#include "../../SDK/L4D2/Interfaces/BaseClientDll.h"
#include "../Logger/Logger.h"

namespace
{
	// Ищем проп по имени; возвращает сам проп и накопленный оффсет.
	RecvProp* FindPropInternal(RecvTable* pTable, const char* const szVar, int& nOutOffset)
	{
		for (int n = 0; n < pTable->GetNumProps(); n++)
		{
			RecvProp* pProp = pTable->GetProp(n);

			if (!pProp)
				continue;

			if (strcmp(szVar, pProp->GetName()) == 0)
			{
				nOutOffset += pProp->GetOffset();
				return pProp;
			}

			RecvTable* pDataTable = pProp->GetDataTable();

			if (!pDataTable)
				continue;

			const int nBase = nOutOffset;
			nOutOffset += pProp->GetOffset();

			if (RecvProp* pFound = FindPropInternal(pDataTable, szVar, nOutOffset))
				return pFound;

			nOutOffset = nBase; // не нашлось во вложенной таблице — откат базы
		}

		return nullptr;
	}
}

int CUtil_NetVarManager::Get(const char* const szClass, const char* const szVar)
{
	return Get(szClass, szVar, 0);
}

int CUtil_NetVarManager::Get(const char* const szClass, const char* const szVar, const int nExpectSize)
{
	ClientClass* pCC = I::BaseClient->GetAllClasses();

	while (pCC)
	{
		if (strcmp(szClass, pCC->m_pNetworkName) == 0)
		{
			int nOffset = 0;
			RecvProp* pProp = FindPropInternal(pCC->m_pRecvTable, szVar, nOffset);

			if (!pProp)
				return 0;

			//RECVINFO кладёт sizeof(поля) в m_StringBufferSize для ВСЕХ типов
			//пропов. Расхождение с объявленным типом = мусорное чтение соседних
			//полей: так байт-проп m_nWaterLevel, объявленный int-ом, рандомно
			//убивал бхоп (мусорный гейт). Лог — чтобы править заголовок.
			if (nExpectSize > 0 && pProp->m_StringBufferSize > 0
				&& pProp->m_StringBufferSize != nExpectSize)
			{
				U::Log.Write("[!] netvar %s.%s: real prop size %d, declared %d — FIX THE SDK TYPE",
					szClass, szVar, pProp->m_StringBufferSize, nExpectSize);
			}

			return nOffset;
		}

		pCC = pCC->m_pNext;
	}

	return 0;
}
