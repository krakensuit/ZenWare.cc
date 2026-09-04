#include "PEHelper.h"

bool CPEHelper::ValidateImage(const std::vector<BYTE>& vecFile)
{
	if (vecFile.size() < sizeof(IMAGE_DOS_HEADER))
		return false;

	const PIMAGE_DOS_HEADER pDos = GetDosHeader(vecFile);

	if (pDos->e_magic != IMAGE_DOS_SIGNATURE) //'MZ'
		return false;

	if (vecFile.size() < (static_cast<size_t>(pDos->e_lfanew) + sizeof(IMAGE_NT_HEADERS)))
		return false;

	const PIMAGE_NT_HEADERS pNt = GetNtHeaders(vecFile);

	if (pNt->Signature != IMAGE_NT_SIGNATURE) //'PE\0\0'
		return false;

	//This mapper is x86-only: the target game is 32-bit.
	if (pNt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		return false;

	if (!(pNt->FileHeader.Characteristics & IMAGE_FILE_DLL))
		return false;

	return true;
}

PIMAGE_DOS_HEADER CPEHelper::GetDosHeader(const std::vector<BYTE>& vecFile)
{
	return reinterpret_cast<PIMAGE_DOS_HEADER>(const_cast<BYTE*>(vecFile.data()));
}

PIMAGE_NT_HEADERS CPEHelper::GetNtHeaders(const std::vector<BYTE>& vecFile)
{
	const PIMAGE_DOS_HEADER pDos = GetDosHeader(vecFile);
	return reinterpret_cast<PIMAGE_NT_HEADERS>(const_cast<BYTE*>(vecFile.data()) + pDos->e_lfanew);
}

DWORD CPEHelper::RvaToOffset(const std::vector<BYTE>& vecFile, DWORD dwRva)
{
	const PIMAGE_NT_HEADERS pNt = GetNtHeaders(vecFile);
	const PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);

	for (WORD i = 0; i < pNt->FileHeader.NumberOfSections; i++)
	{
		const PIMAGE_SECTION_HEADER pSect = &pSection[i];

		if (dwRva >= pSect->VirtualAddress && dwRva < (pSect->VirtualAddress + pSect->Misc.VirtualSize))
		{
			const DWORD dwDelta = pSect->VirtualAddress - pSect->PointerToRawData;
			return (dwRva - dwDelta);
		}
	}

	//Not inside any section: headers region maps identity.
	if (dwRva < pNt->OptionalHeader.SizeOfHeaders)
		return dwRva;

	return 0;
}

bool CPEHelper::GetDataDirectory(const std::vector<BYTE>& vecFile, const int nIndex, DWORD& dwRva, DWORD& dwSize)
{
	const PIMAGE_NT_HEADERS pNt = GetNtHeaders(vecFile);

	if (nIndex < 0 || nIndex >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
		return false;

	dwRva = pNt->OptionalHeader.DataDirectory[nIndex].VirtualAddress;
	dwSize = pNt->OptionalHeader.DataDirectory[nIndex].Size;

	return (dwRva != 0 && dwSize != 0);
}

DWORD CPEHelper::ProtectionFromCharacteristics(DWORD dwCharacteristics)
{
	DWORD dwProtect = PAGE_READONLY;

	if (dwCharacteristics & IMAGE_SCN_MEM_EXECUTE)
	{
		dwProtect = (dwCharacteristics & IMAGE_SCN_MEM_WRITE) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
	}
	else if (dwCharacteristics & IMAGE_SCN_MEM_WRITE)
	{
		dwProtect = PAGE_READWRITE;
	}

	return dwProtect;
}

