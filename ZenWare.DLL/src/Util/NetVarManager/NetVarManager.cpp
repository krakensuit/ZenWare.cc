#include "NetVarManager.h"

#include "../../SDK/L4D2/Includes/dt_recv.h"
#include "../../SDK/L4D2/Interfaces/BaseClientDll.h"
#include "../Logger/Logger.h"

namespace
{
	// Find a prop by name; returns the prop itself and the accumulated offset.
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

			nOutOffset = nBase; // not found in the nested table — roll back the base
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

			//RECVINFO stores sizeof(field) in m_StringBufferSize for ALL prop
			//types. A mismatch with the declared type = garbage reads of
			//neighboring fields: that is how the byte prop m_nWaterLevel, declared
			//as an int, randomly killed bhop (garbage gate). Logged so the header gets fixed.
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
