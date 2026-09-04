#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace LoaderLog
{
	using Fn = void(*)(const HWND, const char* const, ...);
	using StatusFn = void(*)(const HWND, const char* const);
}

class CManualMapper
{
public:
	struct Params_t
	{
		HWND hwndLog = nullptr;
		DWORD dwTargetPid = 0;
		std::wstring wszDllPath = L"";
		LoaderLog::Fn pfnLog = nullptr;
		LoaderLog::StatusFn pfnStatus = nullptr; //short progress text for the status bar
	};

	//Full manual-map pipeline. Returns true on success.
	bool Map(const Params_t& params) const;

	//STANDARD injection via remote LoadLibraryW: the OS loader performs the
	//mapping (TLS, CRT init, module list registration - everything handled by
	//Windows). Maximum compatibility, recommended first choice.
	static bool InjectStandard(const Params_t& params);

private:
	static bool ReadFileToBuffer(HWND hwndLog, LoaderLog::Fn fnLog, const std::wstring& wszPath, std::vector<BYTE>& vecOut);

	//Assembles the in-memory image: headers + every section at its VirtualAddress.
	static bool BuildLocalImage(HWND hwndLog, LoaderLog::Fn fnLog, const std::vector<BYTE>& vecFile, std::vector<BYTE>& vecImage);

	//Patches absolute addresses (IMAGE_REL_BASED_HIGHLOW) for a non-preferred base.
	static bool ApplyRelocations(HWND hwndLog, LoaderLog::Fn fnLog, BYTE* pImage, DWORD dwDelta);

	//Resolves the import table using local module handles / GetProcAddress.
	static bool ResolveImports(HWND hwndLog, LoaderLog::Fn fnLog, BYTE* pImage);

	//Applies per-section page protections in the target process.
	static bool ProtectSections(HWND hwndLog, LoaderLog::Fn fnLog, const HANDLE hProcess, const DWORD dwBase, const PIMAGE_NT_HEADERS pNt);

	//Runs DllMain(base, DLL_PROCESS_ATTACH, NULL) through a small remote stub,
	//then zeroes the first 0x1000 bytes (PE header wipe).
	static bool CallEntryAndWipeHeaders(HWND hwndLog, LoaderLog::Fn fnLog, const HANDLE hProcess, const DWORD dwBase, const PIMAGE_NT_HEADERS pNt);
};
