#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

// Только чтение чужой памяти (RPM). Никаких записей в игру, хуков и инжекта.
class Memory
{
public:
	bool Attach(const wchar_t* procName);
	bool IsOpen() const { return m_hProc != nullptr; }
	void Close();

	bool Module(const wchar_t* name, uintptr_t& base, uint32_t& size) const;

	template<typename T>
	bool Read(uintptr_t addr, T& out) const
	{
		if (!m_hProc || !addr)
			return false;
		SIZE_T rd = 0;
		return ReadProcessMemory(m_hProc, (LPCVOID)addr, &out, sizeof(T), &rd) && rd == sizeof(T);
	}

	bool ReadRaw(uintptr_t addr, void* buf, size_t len) const;

	// IDA-стиль "AA BB ? ? CC" по уже прочитанному буферу модуля.
	static intptr_t Scan(const uint8_t* data, size_t len, const char* pattern);

private:
	HANDLE m_hProc = nullptr;
	DWORD m_pid = 0;
};
