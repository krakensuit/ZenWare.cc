#include "Memory.h"
#include <tlhelp32.h>
#include <sstream>

bool Memory::Attach(const wchar_t* procName)
{
	Close();
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
		return false;

	DWORD pid = 0;
	PROCESSENTRY32W pe = { sizeof(pe) };
	if (Process32FirstW(snap, &pe))
	{
		do
		{
			if (_wcsicmp(pe.szExeFile, procName) == 0) { pid = pe.th32ProcessID; break; }
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
	if (!pid)
		return false;

	m_hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (!m_hProc)
		return false;
	m_pid = pid;
	return true;
}

void Memory::Close()
{
	if (m_hProc) { CloseHandle(m_hProc); m_hProc = nullptr; }
	m_pid = 0;
}

bool Memory::Module(const wchar_t* name, uintptr_t& base, uint32_t& size) const
{
	base = 0; size = 0;
	if (!m_hProc)
		return false;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
	if (snap == INVALID_HANDLE_VALUE)
		return false;
	bool ok = false;
	MODULEENTRY32W me = { sizeof(me) };
	if (Module32FirstW(snap, &me))
	{
		do
		{
			if (_wcsicmp(me.szModule, name) == 0)
			{
				base = (uintptr_t)me.modBaseAddr;
				size = me.modBaseSize;
				ok = true;
				break;
			}
		} while (Module32NextW(snap, &me));
	}
	CloseHandle(snap);
	return ok;
}

bool Memory::ReadRaw(uintptr_t addr, void* buf, size_t len) const
{
	if (!m_hProc || !addr || !buf || !len)
		return false;
	SIZE_T rd = 0;
	return ReadProcessMemory(m_hProc, (LPCVOID)addr, buf, len, &rd) && rd == len;
}

intptr_t Memory::Scan(const uint8_t* data, size_t len, const char* pattern)
{
	std::vector<uint8_t> bytes;
	std::vector<bool> mask;
	std::istringstream ss(pattern);
	std::string tok;
	while (ss >> tok)
	{
		if (tok == "?" || tok == "??") { bytes.push_back(0); mask.push_back(false); }
		else { bytes.push_back((uint8_t)strtoul(tok.c_str(), nullptr, 16)); mask.push_back(true); }
	}
	if (bytes.empty() || bytes.size() > len)
		return -1;
	for (size_t i = 0; i + bytes.size() <= len; ++i)
	{
		bool hit = true;
		for (size_t j = 0; j < bytes.size(); ++j)
		{
			if (mask[j] && data[i + j] != bytes[j]) { hit = false; break; }
		}
		if (hit)
			return (intptr_t)i;
	}
	return -1;
}
