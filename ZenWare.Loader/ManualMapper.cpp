#include "ManualMapper.h"

#include "PEHelper.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
// Manual mapping pipeline (x86 DLL into an x86 process):
//   1. Read the DLL file into a local buffer.
//   2. VirtualAllocEx RWX at the preferred ImageBase (fallback: any base).
//   3. Build the in-memory image locally: headers + sections at their RVA.
//   4. Process IMAGE_BASE_RELOCATION blocks (HIGHLOW) for the base delta.
//   5. Walk the import table. Kernel32/system module base addresses are
//      identical within one boot session across processes, so local
//      GetProcAddress results are written straight into the target IAT.
//   6. One WriteProcessMemory of the assembled image + read-back verification,
//      then VirtualProtectEx per section according to its characteristics.
//   7. A stub written into the target calls DllMain(base, DLL_PROCESS_ATTACH,
//      NULL): CreateRemoteThread passes only ONE argument, so the stub pushes
//      all three DllMain parameters itself before calling the entry point.
//   8. On success the first 0x1000 bytes (headers) are zeroed and verified.
// Every failure path releases the remote allocation.
// ---------------------------------------------------------------------------

namespace
{
	const char* ProtName(const DWORD dwProtect)
	{
		switch (dwProtect)
		{
			case PAGE_NOACCESS: return "NOACCESS";
			case PAGE_READONLY: return "READONLY";
			case PAGE_READWRITE: return "READWRITE";
			case PAGE_WRITECOPY: return "WRITECOPY";
			case PAGE_EXECUTE: return "EXECUTE";
			case PAGE_EXECUTE_READ: return "EXECUTE_READ";
			case PAGE_EXECUTE_READWRITE: return "EXECUTE_RW";
			case PAGE_EXECUTE_WRITECOPY: return "EXECUTE_WC";
			default: return "UNKNOWN";
		}
	}

	struct StageTimer_t
	{
		ULONGLONG m_nStart = 0;

		void Begin() { m_nStart = GetTickCount64(); }
		ULONGLONG ElapsedMs() const { return GetTickCount64() - m_nStart; }
	};
}

bool CManualMapper::Map(const Params_t& params) const
{
	const HWND hwndLog = params.hwndLog;
	const auto fnLog = params.pfnLog;
	const auto fnStatus = params.pfnStatus;

	if (!fnLog || !fnStatus)
		return false;

	const ULONGLONG nTotalStart = GetTickCount64();

	fnLog(hwndLog, "================ INJECTION START ================");
	fnLog(hwndLog, "target PID : %lu", params.dwTargetPid);
	fnLog(hwndLog, "dll path   : %ls", params.wszDllPath.c_str());

	std::vector<BYTE> vecFile;
	bool bFromResource = false;

	fnStatus(hwndLog, LoaderUtil::S("Чтение файла", "Reading file"));
	StageTimer_t t;
	t.Begin();

	if (!ReadFileToBuffer(hwndLog, fnLog, params.wszDllPath, vecFile))
	{
		// Файл не найден — пробуем встроенный ресурс (для однофайловой раздачи)
		if (!LoaderUtil::LoadDllFromResource(vecFile))
		{
			fnLog(hwndLog, "[!] Failed to read the DLL file and no embedded resource found.");
			return false;
		}
		bFromResource = true;
		fnLog(hwndLog, "[*] Loaded DLL from embedded resource: %u bytes (%lu ms).", static_cast<unsigned>(vecFile.size()), static_cast<unsigned long>(t.ElapsedMs()));
	}
	else
	{
		fnLog(hwndLog, "[+] File read: %u bytes (%lu ms).", static_cast<unsigned>(vecFile.size()), static_cast<unsigned long>(t.ElapsedMs()));
	}

	fnStatus(hwndLog, LoaderUtil::S("Проверка PE", "Checking PE headers"));

	if (!CPEHelper::ValidateImage(vecFile))
	{
		fnLog(hwndLog, "[!] Invalid PE image or not an x86 DLL.");
		return false;
	}

	const PIMAGE_NT_HEADERS pNt = CPEHelper::GetNtHeaders(vecFile);

	fnLog(hwndLog, "[+] PE ok: SizeOfImage=0x%08X ImageBase=0x%08X EP=0x%08X sections=%u",
		pNt->OptionalHeader.SizeOfImage, pNt->OptionalHeader.ImageBase,
		pNt->OptionalHeader.AddressOfEntryPoint, pNt->FileHeader.NumberOfSections);

	fnStatus(hwndLog, LoaderUtil::S("Сборка образа", "Building image"));
	t.Begin();

	std::vector<BYTE> vecImage;

	if (!BuildLocalImage(hwndLog, fnLog, vecFile, vecImage))
	{
		fnLog(hwndLog, "[!] Section layout is corrupt (out of bounds).");
		return false;
	}

	fnLog(hwndLog, "[+] Local image built in %lu ms.", static_cast<unsigned long>(t.ElapsedMs()));

	fnStatus(hwndLog, LoaderUtil::S("Выделение памяти", "Allocating memory"));

	const HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, params.dwTargetPid);

	if (!hProcess)
	{
		fnLog(hwndLog, "[!] OpenProcess failed, error %lu.", GetLastError());
		return false;
	}

	wchar_t wszTarget[MAX_PATH] = { };
	DWORD dwTargetLen = MAX_PATH;

	if (QueryFullProcessImageNameW(hProcess, 0, wszTarget, &dwTargetLen))
		fnLog(hwndLog, "[+] Target process: %ls", wszTarget);

	bool bResult = false;

	t.Begin();
	DWORD dwBase = reinterpret_cast<DWORD>(VirtualAllocEx(hProcess, reinterpret_cast<LPVOID>(pNt->OptionalHeader.ImageBase), pNt->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));

	if (dwBase)
		fnLog(hwndLog, "[+] Allocated at preferred base 0x%08X.", dwBase);
	else
	{
		dwBase = reinterpret_cast<DWORD>(VirtualAllocEx(hProcess, nullptr, pNt->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));

		if (!dwBase)
		{
			fnLog(hwndLog, "[!] VirtualAllocEx failed, error %lu.", GetLastError());
			CloseHandle(hProcess);
			return false;
		}

		fnLog(hwndLog, "[*] Preferred base was taken, allocated at 0x%08X instead.", dwBase);
	}

	fnLog(hwndLog, "[*] Allocation took %lu ms.", static_cast<unsigned long>(t.ElapsedMs()));

	do
	{
		const DWORD dwDelta = dwBase - pNt->OptionalHeader.ImageBase;

		fnStatus(hwndLog, LoaderUtil::S("Релокации", "Applying relocations"));
		t.Begin();

		if (dwDelta != 0)
		{
			if (!ApplyRelocations(hwndLog, fnLog, vecImage.data(), dwDelta))
			{
				fnLog(hwndLog, "[!] Relocation processing failed.");
				break;
			}
		}
		else
		{
			fnLog(hwndLog, "[*] No relocations needed (delta is zero).");
		}

		fnLog(hwndLog, "[*] Relocation stage took %lu ms.", static_cast<unsigned long>(t.ElapsedMs()));

		fnStatus(hwndLog, LoaderUtil::S("Импорты", "Resolving imports"));
		t.Begin();

		if (!ResolveImports(hwndLog, fnLog, vecImage.data()))
		{
			fnLog(hwndLog, "[!] Import resolution failed.");
			break;
		}

		fnLog(hwndLog, "[+] Imports resolved in %lu ms.", static_cast<unsigned long>(t.ElapsedMs()));

		fnStatus(hwndLog, LoaderUtil::S("Запись в процесс", "Writing image"));
		t.Begin();

		SIZE_T nWritten = 0;

		if (!WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(dwBase), vecImage.data(), pNt->OptionalHeader.SizeOfImage, &nWritten))
		{
			fnLog(hwndLog, "[!] WriteProcessMemory failed, error %lu.", GetLastError());
			break;
		}

		fnLog(hwndLog, "[+] Written %u / %u bytes.", static_cast<unsigned>(nWritten), pNt->OptionalHeader.SizeOfImage);

		//Read-back verification: proves AV/EDR did not silently block the write.
		{
			std::vector<BYTE> vecVerify(pNt->OptionalHeader.SizeOfImage);
			SIZE_T nRead = 0;

			if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(dwBase), vecVerify.data(), vecVerify.size(), &nRead))
			{
				fnLog(hwndLog, "[!] Read-back failed, error %lu.", GetLastError());
				break;
			}

			if (nRead != vecVerify.size())
			{
				fnLog(hwndLog, "[!] Read-back short: %u of %u bytes.", static_cast<unsigned>(nRead), static_cast<unsigned>(vecVerify.size()));
				break;
			}

			if (memcmp(vecVerify.data(), vecImage.data(), vecVerify.size()) != 0)
			{
				for (size_t i = 0; i < vecVerify.size(); i++)
				{
					if (vecVerify[i] != vecImage[i])
					{
						fnLog(hwndLog, "[!] Verify MISMATCH at image+0x%08X (file 0x%02X != remote 0x%02X).",
							static_cast<unsigned>(i), vecImage[i], vecVerify[i]);
						break;
					}
				}
				break;
			}

			fnLog(hwndLog, "[+] Read-back verification passed (%u bytes identical).", static_cast<unsigned>(nRead));
		}

		fnLog(hwndLog, "[*] Write+verify took %lu ms.", static_cast<unsigned long>(t.ElapsedMs()));

		fnStatus(hwndLog, LoaderUtil::S("Права секций", "Protecting sections"));
		t.Begin();

		if (!ProtectSections(hwndLog, fnLog, hProcess, dwBase, pNt))
		{
			fnLog(hwndLog, "[!] VirtualProtectEx failed, error %lu.", GetLastError());
			break;
		}

		fnLog(hwndLog, "[*] Protections took %lu ms.", static_cast<unsigned long>(t.ElapsedMs()));

		fnStatus(hwndLog, LoaderUtil::S("Запуск DllMain", "Calling DllMain"));

		if (!CallEntryAndWipeHeaders(hwndLog, fnLog, hProcess, dwBase, pNt))
		{
			fnLog(hwndLog, "[!] Entry point call failed.");
			break;
		}

		fnStatus(hwndLog, LoaderUtil::S("Готово", "Done"));
		fnLog(hwndLog, "[===] Injected successfully at 0x%08X (total %lu ms).", dwBase, static_cast<unsigned long>(GetTickCount64() - nTotalStart));
		bResult = true;
	} while (false);

	if (!bResult)
		VirtualFreeEx(hProcess, reinterpret_cast<LPVOID>(dwBase), 0, MEM_RELEASE);

	CloseHandle(hProcess);
	return bResult;
}

bool CManualMapper::ReadFileToBuffer(HWND hwndLog, LoaderLog::Fn fnLog, const std::wstring& wszPath, std::vector<BYTE>& vecOut)
{
	const HANDLE hFile = CreateFileW(wszPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		fnLog(hwndLog, "[!] CreateFileW failed, error %lu.", GetLastError());
		return false;
	}

	const DWORD dwSize = GetFileSize(hFile, nullptr);

	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		fnLog(hwndLog, "[!] File is empty or too big.");
		CloseHandle(hFile);
		return false;
	}

	vecOut.resize(dwSize);

	const BOOL bRead = ReadFile(hFile, vecOut.data(), dwSize, nullptr, nullptr);
	CloseHandle(hFile);

	if (!bRead)
		fnLog(hwndLog, "[!] ReadFile failed, error %lu.", GetLastError());

	return bRead != FALSE;
}

bool CManualMapper::BuildLocalImage(HWND hwndLog, LoaderLog::Fn fnLog, const std::vector<BYTE>& vecFile, std::vector<BYTE>& vecImage)
{
	const PIMAGE_NT_HEADERS pNt = CPEHelper::GetNtHeaders(vecFile);

	vecImage.assign(pNt->OptionalHeader.SizeOfImage, 0);

	memcpy(vecImage.data(), vecFile.data(), pNt->OptionalHeader.SizeOfHeaders);
	fnLog(hwndLog, "    headers    : 0x%08X bytes -> image+0", pNt->OptionalHeader.SizeOfHeaders);

	const PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);

	for (WORD i = 0; i < pNt->FileHeader.NumberOfSections; i++)
	{
		const PIMAGE_SECTION_HEADER pSect = &pSection[i];

		char szName[9] = { };
		memcpy(szName, pSect->Name, 8);

		if (pSect->PointerToRawData == 0 || pSect->SizeOfRawData == 0)
		{
			fnLog(hwndLog, "    %-8s : empty (bss-like), virtual 0x%08X stays zeroed", szName, pSect->VirtualAddress);
			continue;
		}

		if ((static_cast<size_t>(pSect->PointerToRawData) + pSect->SizeOfRawData) > vecFile.size())
		{
			fnLog(hwndLog, "[!] Section %.8s raw range out of file bounds.", szName);
			return false;
		}

		if ((static_cast<size_t>(pSect->VirtualAddress) + pSect->SizeOfRawData) > vecImage.size())
		{
			fnLog(hwndLog, "[!] Section %.8s exceeds SizeOfImage.", szName);
			return false;
		}

		memcpy(vecImage.data() + pSect->VirtualAddress, vecFile.data() + pSect->PointerToRawData, pSect->SizeOfRawData);

		fnLog(hwndLog, "    %-8s : file 0x%08X (%7u b) -> image+0x%08X (virt %7u b)",
			szName, pSect->PointerToRawData, pSect->SizeOfRawData, pSect->VirtualAddress, pSect->Misc.VirtualSize);
	}

	return true;
}

bool CManualMapper::ApplyRelocations(HWND hwndLog, LoaderLog::Fn fnLog, BYTE* pImage, DWORD dwDelta)
{
	const PIMAGE_DOS_HEADER pDos = reinterpret_cast<PIMAGE_DOS_HEADER>(pImage);
	const PIMAGE_NT_HEADERS pNt = reinterpret_cast<PIMAGE_NT_HEADERS>(pImage + pDos->e_lfanew);

	const IMAGE_DATA_DIRECTORY& dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

	if (!dir.VirtualAddress || !dir.Size)
	{
		fnLog(hwndLog, "[*] Image has no relocation directory.");
		return true;
	}

	if (dir.VirtualAddress > pNt->OptionalHeader.SizeOfImage || dir.Size > pNt->OptionalHeader.SizeOfImage - dir.VirtualAddress)
	{
		fnLog(hwndLog, "[!] Relocation directory out of image bounds.");
		return false;
	}

	const DWORD dwImageSize = pNt->OptionalHeader.SizeOfImage;
	BYTE* pRelocBlob = pImage + dir.VirtualAddress;
	const BYTE* pBlobEnd = pRelocBlob + dir.Size;

	DWORD dwBlocks = 0, dwPatched = 0, dwSkipped = 0;

	while (pRelocBlob < pBlobEnd)
	{
		const PIMAGE_BASE_RELOCATION pBlock = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pRelocBlob);

		if (pBlock->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION))
		{
			fnLog(hwndLog, "[!] Corrupt relocation block size %u.", pBlock->SizeOfBlock);
			return false;
		}

		const DWORD dwCount = (pBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
		WORD* pwEntries = reinterpret_cast<WORD*>(pRelocBlob + sizeof(IMAGE_BASE_RELOCATION));

		for (DWORD i = 0; i < dwCount; i++)
		{
			const WORD wType = (pwEntries[i] >> 12) & 0xF;
			const WORD wOffset = pwEntries[i] & 0xFFF;

			switch (wType)
			{
				case IMAGE_REL_BASED_ABSOLUTE: //padding, skip
					dwSkipped++;
					break;

				case IMAGE_REL_BASED_HIGHLOW:
				{
					const DWORD dwTgtRva = pBlock->VirtualAddress + wOffset;

					if (dwTgtRva > dwImageSize || dwImageSize - dwTgtRva < sizeof(DWORD))
					{
						fnLog(hwndLog, "[!] Relocation target out of bounds (rva 0x%08X).", dwTgtRva);
						return false;
					}

					DWORD* pdwAddr = reinterpret_cast<DWORD*>(pImage + dwTgtRva);
					*pdwAddr += dwDelta;
					dwPatched++;
					break;
				}

				default: //DIR64 etc. never occur in x86 images
					fnLog(hwndLog, "[!] Unsupported reloc type %u (block VA 0x%08X).", wType, pBlock->VirtualAddress);
					return false;
			}
		}

		dwBlocks++;
		pRelocBlob += pBlock->SizeOfBlock;
	}

	fnLog(hwndLog, "[+] Relocations: delta 0x%08X, blocks=%lu, HIGHLOW patched=%lu, padding skipped=%lu.",
		dwDelta, dwBlocks, dwPatched, dwSkipped);
	return true;
}

bool CManualMapper::ResolveImports(HWND hwndLog, LoaderLog::Fn fnLog, BYTE* pImage)
{
	const PIMAGE_DOS_HEADER pDos = reinterpret_cast<PIMAGE_DOS_HEADER>(pImage);
	const PIMAGE_NT_HEADERS pNt = reinterpret_cast<PIMAGE_NT_HEADERS>(pImage + pDos->e_lfanew);

	const IMAGE_DATA_DIRECTORY& dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	if (!dir.VirtualAddress || !dir.Size)
	{
		fnLog(hwndLog, "[*] Image has no imports.");
		return true;
	}

	auto* pDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pImage + dir.VirtualAddress);

	DWORD dwTotalModules = 0, dwTotalFuncs = 0;

	for (; pDesc->Name != 0; ++pDesc, ++dwTotalModules)
	{
		char* szModuleName = reinterpret_cast<char*>(pImage + pDesc->Name);

		//System modules are mapped at identical addresses in every process
		//of the current session, so a local handle value is valid remotely.
		HMODULE hModule = GetModuleHandleA(szModuleName);

		if (!hModule)
			hModule = LoadLibraryA(szModuleName); //loads into THIS process only

		if (!hModule)
		{
			fnLog(hwndLog, "[!] Import module not found locally: %s", szModuleName);
			return false;
		}

		auto* pOrig = reinterpret_cast<IMAGE_THUNK_DATA*>(pImage + (pDesc->OriginalFirstThunk ? pDesc->OriginalFirstThunk : pDesc->FirstThunk));
		auto* pFirst = reinterpret_cast<IMAGE_THUNK_DATA*>(pImage + pDesc->FirstThunk);

		DWORD dwCount = 0;

		for (; pOrig->u1.AddressOfData != 0; ++pOrig, ++pFirst, ++dwCount, ++dwTotalFuncs)
		{
			FARPROC pFunction = nullptr;
			char szFn[96] = { };

			if (IMAGE_SNAP_BY_ORDINAL32(pOrig->u1.Ordinal))
			{
				const WORD wOrd = IMAGE_ORDINAL32(pOrig->u1.Ordinal);
				sprintf_s(szFn, "#%u", wOrd);
				pFunction = GetProcAddress(hModule, MAKEINTRESOURCEA(wOrd));
			}
			else
			{
				auto* pByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pImage + pOrig->u1.AddressOfData);
				strncpy_s(szFn, reinterpret_cast<char*>(pByName->Name), _TRUNCATE);
				pFunction = GetProcAddress(hModule, reinterpret_cast<char*>(pByName->Name));
			}

			if (!pFunction)
			{
				fnLog(hwndLog, "[!] Function not found in %s: %s", szModuleName, szFn);
				return false;
			}

			fnLog(hwndLog, "      %s!%s -> 0x%08X", szModuleName, szFn, reinterpret_cast<DWORD>(pFunction));
			pFirst->u1.Function = reinterpret_cast<DWORD>(pFunction);
		}

		fnLog(hwndLog, "    [%2lu] %-24s : %lu functions", dwTotalModules + 1, szModuleName, dwCount);
	}

	fnLog(hwndLog, "[+] Total: %lu modules, %lu imports resolved into IAT.", dwTotalModules, dwTotalFuncs);
	return true;
}

bool CManualMapper::ProtectSections(HWND hwndLog, LoaderLog::Fn fnLog, const HANDLE hProcess, const DWORD dwBase, const PIMAGE_NT_HEADERS pNt)
{
	DWORD dwOld = 0;

	if (!VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(dwBase), pNt->OptionalHeader.SizeOfHeaders, PAGE_READONLY, &dwOld))
		return false;

	fnLog(hwndLog, "    headers    : READONLY (was 0x%08X)", dwOld);

	const PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);

	for (WORD i = 0; i < pNt->FileHeader.NumberOfSections; i++)
	{
		const PIMAGE_SECTION_HEADER pSect = &pSection[i];

		char szName[9] = { };
		memcpy(szName, pSect->Name, 8);

		if (pSect->Misc.VirtualSize == 0)
			continue;

		LPVOID pAddr = reinterpret_cast<LPVOID>(dwBase + pSect->VirtualAddress);
		const DWORD dwRegionSize = (pSect->Misc.VirtualSize + 0xFFF) & ~0xFFF; //round up to page
		const DWORD dwWant = CPEHelper::ProtectionFromCharacteristics(pSect->Characteristics);

		if (!VirtualProtectEx(hProcess, pAddr, dwRegionSize, dwWant, &dwOld))
			return false;

		fnLog(hwndLog, "    %-8s : %-13s (%7u b) at 0x%08X", szName, ProtName(dwWant), dwRegionSize, dwBase + pSect->VirtualAddress);
	}

	return true;
}

bool CManualMapper::CallEntryAndWipeHeaders(HWND hwndLog, LoaderLog::Fn fnLog, const HANDLE hProcess, const DWORD dwBase, const PIMAGE_NT_HEADERS pNt)
{
	const DWORD dwEntry = dwBase + pNt->OptionalHeader.AddressOfEntryPoint;

	//x86 stub:
	//   push 0                    ; Reserved
	//   push 1                    ; fdwReason = DLL_PROCESS_ATTACH
	//   push dwBase               ; hinstDLL
	//   mov eax, dwEntry
	//   call eax                  ; DllMain (__stdcall, cleans its own args)
	//   ret                       ; back to the thread dispatcher
	BYTE abyStub[22] = { };
	size_t nIdx = 0;

	abyStub[nIdx++] = 0x68; //push imm32 (Reserved = 0, buffer already zero)
	nIdx += 4;

	abyStub[nIdx++] = 0x68; //push imm32
	const DWORD dwReason = DLL_PROCESS_ATTACH;
	memcpy(abyStub + nIdx, &dwReason, sizeof(dwReason));
	nIdx += sizeof(dwReason);

	abyStub[nIdx++] = 0x68; //push imm32
	memcpy(abyStub + nIdx, &dwBase, sizeof(dwBase));
	nIdx += sizeof(dwBase);

	abyStub[nIdx++] = 0xB8; //mov eax, imm32
	memcpy(abyStub + nIdx, &dwEntry, sizeof(dwEntry));
	nIdx += sizeof(dwEntry);

	abyStub[nIdx++] = 0xFF; //call eax
	abyStub[nIdx++] = 0xD0;

	abyStub[nIdx++] = 0xC3; //ret

	fnLog(hwndLog, "[*] Stub: %u bytes, entry target 0x%08X.", static_cast<unsigned>(nIdx), dwEntry);

	LPVOID pRemoteStub = VirtualAllocEx(hProcess, nullptr, sizeof(abyStub), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (!pRemoteStub)
	{
		fnLog(hwndLog, "[!] Stub allocation failed, error %lu.", GetLastError());
		return false;
	}

	if (!WriteProcessMemory(hProcess, pRemoteStub, abyStub, sizeof(abyStub), nullptr))
	{
		fnLog(hwndLog, "[!] Stub write failed, error %lu.", GetLastError());
		VirtualFreeEx(hProcess, pRemoteStub, 0, MEM_RELEASE);
		return false;
	}

	const ULONGLONG nThreadStart = GetTickCount64();
	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pRemoteStub), nullptr, 0, nullptr);

	if (!hThread)
	{
		fnLog(hwndLog, "[!] CreateRemoteThread failed, error %lu.", GetLastError());
		VirtualFreeEx(hProcess, pRemoteStub, 0, MEM_RELEASE);
		return false;
	}

	fnLog(hwndLog, "[+] Remote thread launched (id %lu). Waiting up to 10 s ...", GetThreadId(hThread));

	//DllMain spawns its own init thread and returns quickly.
	const DWORD dwWait = WaitForSingleObject(hThread, 10 * 1000);
	DWORD dwExit = 0;
	GetExitCodeThread(hThread, &dwExit);
	CloseHandle(hThread);

	switch (dwWait)
	{
		case WAIT_OBJECT_0:
			fnLog(hwndLog, "[+] Thread finished in %lu ms, exit code %lu (DllMain TRUE expected).",
				static_cast<unsigned long>(GetTickCount64() - nThreadStart), dwExit);
			break;
		case WAIT_TIMEOUT:
			fnLog(hwndLog, "[!] Thread still running after 10 s (init thread may still work). exit=%lu", dwExit);
			break;
		default:
			fnLog(hwndLog, "[!] WaitForSingleObject returned 0x%08X.", dwWait);
			break;
	}

	VirtualFreeEx(hProcess, pRemoteStub, 0, MEM_RELEASE);

	//Wipe the PE headers: zero the first page(s) and mark them read-only.
	const SIZE_T nWipeLen = (pNt->OptionalHeader.SizeOfHeaders < 0x1000) ? pNt->OptionalHeader.SizeOfHeaders : 0x1000;
	std::vector<BYTE> vecZeros(nWipeLen, 0);
	DWORD dwOld = 0;

	if (!VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(dwBase), nWipeLen, PAGE_READWRITE, &dwOld))
	{
		fnLog(hwndLog, "[!] Header wipe: VirtualProtectEx failed.");
		return false;
	}

	BOOL bWiped = WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(dwBase), vecZeros.data(), nWipeLen, nullptr);
	VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(dwBase), nWipeLen, PAGE_READONLY, &dwOld);

	if (!bWiped)
	{
		fnLog(hwndLog, "[!] Header wipe: WriteProcessMemory failed.");
		return false;
	}

	//Verify the wipe actually landed.
	{
		std::vector<BYTE> vecCheck(nWipeLen);
		ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(dwBase), vecCheck.data(), nWipeLen, nullptr);

		bool bAllZero = true;

		for (const BYTE by : vecCheck)
		{
			if (by != 0) { bAllZero = false; break; }
		}

		fnLog(hwndLog, (bAllZero ? "[+] Headers wiped and verified (%u zero bytes)." : "[!] Header wipe NOT verified (non-zero bytes remain)!"),
			static_cast<unsigned>(nWipeLen));
	}

	return true;
}


// ---------------------------------------------------------------------------
// STANDARD injection: remote LoadLibraryW(path). The Windows loader inside
// the target process does the mapping itself, so TLS, CRT initializers and
// the module list are all handled exactly like for a normal DLL. This is
// the most compatible route (same principle as Extreme Injector's Standard).
// ---------------------------------------------------------------------------

bool CManualMapper::InjectStandard(const Params_t& params)
{
	const HWND hwndLog = params.hwndLog;
	const auto fnLog = params.pfnLog;
	const auto fnStatus = params.pfnStatus;

	if (!fnLog || !fnStatus)
		return false;

	const ULONGLONG nTotalStart = GetTickCount64();

	fnLog(hwndLog, "================ STANDARD INJECTION (LoadLibraryW) ================");

	const HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, params.dwTargetPid);

	if (!hProcess)
	{
		fnLog(hwndLog, "[!] OpenProcess failed, error %lu.", GetLastError());
		return false;
	}

	bool bResult = false;
	LPVOID pRemotePath = nullptr;

	do
	{
		fnStatus(hwndLog, LoaderUtil::S("Запись пути DLL", "Writing DLL path"));

		const SIZE_T nBytes = (params.wszDllPath.size() + 1) * sizeof(wchar_t);
		pRemotePath = VirtualAllocEx(hProcess, nullptr, nBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		if (!pRemotePath)
		{
			fnLog(hwndLog, "[!] Path allocation failed, error %lu.", GetLastError());
			break;
		}

		if (!WriteProcessMemory(hProcess, pRemotePath, params.wszDllPath.c_str(), nBytes, nullptr))
		{
			fnLog(hwndLog, "[!] Path write failed, error %lu.", GetLastError());
			break;
		}

		wchar_t wszCheck[MAX_PATH] = { };
		ReadProcessMemory(hProcess, pRemotePath, wszCheck, nBytes, nullptr);
		fnLog(hwndLog, "[+] Path written and read back: %ls", wszCheck);

		fnStatus(hwndLog, LoaderUtil::S("Вызов LoadLibraryW", "Calling LoadLibraryW"));

		const HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
		const LPTHREAD_START_ROUTINE pLoadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel32, "LoadLibraryW"));

		if (!pLoadLibraryW)
		{
			fnLog(hwndLog, "[!] LoadLibraryW not found in kernel32.");
			break;
		}

		HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, pLoadLibraryW, pRemotePath, 0, nullptr);

		if (!hThread)
		{
			fnLog(hwndLog, "[!] CreateRemoteThread failed, error %lu.", GetLastError());
			break;
		}

		const DWORD dwWait = WaitForSingleObject(hThread, 10 * 1000);
		DWORD dwModuleBase = 0;
		GetExitCodeThread(hThread, &dwModuleBase);
		CloseHandle(hThread);

		if (dwWait != WAIT_OBJECT_0)
		{
			fnLog(hwndLog, "[!] LoadLibrary thread did not finish in 10 s.");
			break;
		}

		if (!dwModuleBase)
		{
			fnLog(hwndLog, "[!] LoadLibraryW returned NULL inside the target (check dependencies).");
			break;
		}

		fnLog(hwndLog, "[+] Loaded at 0x%08X (total %lu ms).", dwModuleBase, static_cast<unsigned long>(GetTickCount64() - nTotalStart));
		fnStatus(hwndLog, LoaderUtil::S("Готово", "Done"));
		bResult = true;
	} while (false);

	if (pRemotePath)
		VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);

	CloseHandle(hProcess);
	return bResult;
}
