#pragma once

#include <windows.h>
#include <string>
#include <vector>

//Thin helpers around the PE format (x86 images only).
class CPEHelper
{
public:
	//Validates DOS/NT headers and that the image is a 32-bit DLL.
	static bool ValidateImage(const std::vector<BYTE>& vecFile);

	static PIMAGE_DOS_HEADER GetDosHeader(const std::vector<BYTE>& vecFile);
	static PIMAGE_NT_HEADERS GetNtHeaders(const std::vector<BYTE>& vecFile);

	//Translates an RVA into a file-buffer offset using the section table.
	static DWORD RvaToOffset(const std::vector<BYTE>& vecFile, DWORD dwRva);

	//DataDirectory virtual address / size pair.
	static bool GetDataDirectory(const std::vector<BYTE>& vecFile, const int nIndex, DWORD& dwRva, DWORD& dwSize);

	//Maps section characteristics to a page protection value.
	static DWORD ProtectionFromCharacteristics(DWORD dwCharacteristics);
};
