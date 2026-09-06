#include "Updater.h"
#include "Utils.h"
#include "resource.h"
#include <winhttp.h>
#include <string>
#include <vector>
#pragma comment(lib, "winhttp.lib")

namespace {
constexpr wchar_t kApiHost[] = L"api.github.com";
constexpr wchar_t kApiPath[] = L"/repos/krakensuit/ZenWare.cc/releases/latest";
constexpr DWORD kTimeoutMs = 12000;
constexpr size_t kApiCap = 65536;
constexpr size_t kExeCap = 32 * 1024 * 1024;

// "3.3" / "v3.4-beta" -> (3,4). Мусор в конце игнорируется.
bool ParseVer(const wchar_t* s, int& maj, int& mn) {
	maj = 0; mn = 0;
	if (!s) return false;
	while (*s && (*s < L'0' || *s > L'9')) s++;
	if (!*s) return false;
	maj = wcstol(s, nullptr, 10);
	const wchar_t* dot = wcschr(s, L'.');
	if (dot) mn = wcstol(dot + 1, nullptr, 10);
	return true;
}

bool CurrentVer(int& maj, int& mn) {
	wchar_t w[32] = { };
	MultiByteToWideChar(CP_UTF8, 0, ZENWARE_VER_STR, -1, w, 32);
	return ParseVer(w, maj, mn);
}

// Минимальный GET по HTTPS с ручным проходом редиректов (WinHTTP сам не ходит).
// Возвращает тело ответа; outFinalUrl — куда в итоге пришли (для диагностики).
bool HttpsGet(const std::wstring& host, const std::wstring& pathQuery, std::vector<BYTE>& body, size_t cap, std::wstring* outFinalUrl = nullptr) {
	body.clear();
	std::wstring h = host, pq = pathQuery;
	for (int redir = 0; redir < 4; redir++) {
		HINTERNET ses = WinHttpOpen(L"ZenWareUpdater", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!ses) return false;
		WinHttpSetTimeouts(ses, kTimeoutMs, kTimeoutMs, kTimeoutMs, kTimeoutMs);
		bool ok = false;
		HINTERNET con = WinHttpConnect(ses, h.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
		if (con) {
			HINTERNET req = WinHttpOpenRequest(con, L"GET", pq.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
			if (req) {
				// GitHub API без User-Agent отвечает 403.
				WinHttpAddRequestHeaders(req, L"User-Agent: ZenWareUpdater", -1, WINHTTP_ADDREQ_FLAG_ADD);
				if (WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
					WinHttpReceiveResponse(req, nullptr)) {
					DWORD status = 0, sz = sizeof(status);
					WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
					if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
						wchar_t loc[2048] = { };
						DWORD locSz = sizeof(loc);
						if (WinHttpQueryHeaders(req, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, loc, &locSz, WINHTTP_NO_HEADER_INDEX)) {
							std::wstring u = loc;
							size_t p = u.find(L"://");
							if (p != std::wstring::npos) {
								size_t hs = p + 3;
								size_t pe = u.find(L'/', hs);
								if (pe != std::wstring::npos) { h = u.substr(hs, pe - hs); pq = u.substr(pe); }
								else { h = u.substr(hs); pq = L"/"; }
								WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
								continue; // следующий виток по новому адресу
							}
						}
					}
					else if (status == 200) {
						for (;;) {
							BYTE buf[8192];
							DWORD got = 0;
							if (!WinHttpReadData(req, buf, sizeof(buf), &got) || !got) break;
							if (body.size() + got > cap) break;
							body.insert(body.end(), buf, buf + got);
						}
						ok = !body.empty();
						if (outFinalUrl) *outFinalUrl = L"https://" + h + pq;
					}
				}
				WinHttpCloseHandle(req);
			}
			WinHttpCloseHandle(con);
		}
		WinHttpCloseHandle(ses);
		return ok;
	}
	return false;
}

// Вытащить "tag_name": "vX" и первый browser_download_url на .exe из JSON releases/latest.
bool ParseRelease(const std::string& json, std::wstring& tag, std::wstring& exeUrl) {
	tag.clear(); exeUrl.clear();
	auto grabStr = [&](const char* key, size_t from, std::string& v) -> size_t {
		size_t k = json.find(key, from);
		if (k == std::string::npos) return std::string::npos;
		size_t q1 = json.find('"', k + strlen(key));
		if (q1 == std::string::npos) return std::string::npos;
		size_t q2 = json.find('"', q1 + 1);
		if (q2 == std::string::npos) return std::string::npos;
		v.assign(json, q1 + 1, q2 - q1 - 1);
		return q2;
	};
	std::string t;
	if (grabStr("\"tag_name\"", 0, t) == std::string::npos || t.empty()) return false;
	wchar_t wt[64] = { };
	MultiByteToWideChar(CP_UTF8, 0, t.c_str(), -1, wt, 64);
	tag = wt;
	size_t pos = 0;
	for (;;) {
		std::string u;
		pos = grabStr("\"browser_download_url\"", pos, u);
		if (pos == std::string::npos) break;
		pos++;
		if (u.size() > 4 && !u.compare(u.size() - 4, 4, ".exe")) {
			// query-строку отрезаем: WinHTTP сам не всегда её любит, а asset и так резолвится;
			// нет — query НУЖЕН для подписи S3. Оставляем как есть.
			wchar_t wu[2048] = { };
			MultiByteToWideChar(CP_UTF8, 0, u.c_str(), -1, wu, 2048);
			exeUrl = wu;
			break;
		}
	}
	return !exeUrl.empty();
}

bool SplitUrl(const std::wstring& url, std::wstring& host, std::wstring& pathQuery) {
	size_t p = url.find(L"://");
	if (p == std::wstring::npos) return false;
	size_t hs = p + 3;
	size_t pe = url.find(L'/', hs);
	if (pe == std::wstring::npos) { host = url.substr(hs); pathQuery = L"/"; }
	else { host = url.substr(hs, pe - hs); pathQuery = url.substr(pe); }
	return !host.empty();
}

void SafeFileName(std::wstring& s) {
	for (auto& c : s)
		if (c == L'/' || c == L'\\' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|')
			c = L'_';
}

DWORD ThreadBody(HWND hMain) {
	Sleep(3000); // дать UI спокойно открыться
	std::vector<BYTE> api;
	if (!HttpsGet(kApiHost, kApiPath, api, kApiCap)) {
		LoaderUtil::Log(hMain, "[updater] no connection or no releases yet");
		return 0;
	}
		std::string json((char*)api.data(), api.size());
		std::wstring tag, exeUrl;
		if (!ParseRelease(json, tag, exeUrl)) {
			LoaderUtil::Log(hMain, "[updater] latest has no .exe asset");
			return 0;
		}
		int cMaj = 0, cMin = 0, rMaj = 0, rMin = 0;
		CurrentVer(cMaj, cMin);
		ParseVer(tag.c_str(), rMaj, rMin);
		LoaderUtil::Log(hMain, "[updater] current %d.%d, latest %ls (%d.%d)", cMaj, cMin, tag.c_str(), rMaj, rMin);
		if (rMaj < cMaj || (rMaj == cMaj && rMin <= cMin))
			return 0; // мы новее или равны
		LoaderUtil::Status(hMain, LoaderUtil::S("Найдено обновление", "Update available"));
		wchar_t q[256] = { };
		swprintf_s(q, LoaderUtil::SW(L"Доступна версия %ls. Скачать и запустить?", L"Version %ls is available. Download and run?"), tag.c_str());
		if (MessageBoxW(hMain, q, L"ZenWare", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			LoaderUtil::Status(hMain, LoaderUtil::S("Обновление пропущено", "Update skipped"));
			return 0;
		}
		LoaderUtil::Status(hMain, LoaderUtil::S("Скачивание обновления...", "Downloading update..."));
		std::wstring host, pq;
		if (!SplitUrl(exeUrl, host, pq)) return 0;
		std::vector<BYTE> exe;
		if (!HttpsGet(host, pq, exe, kExeCap) || exe.size() < 2 * 1024 * 1024 || exe[0] != 'M' || exe[1] != 'Z') {
			LoaderUtil::Log(hMain, "[updater] download failed or bad file");
			LoaderUtil::Status(hMain, LoaderUtil::S("Ошибка скачивания", "Download failed"));
			return 0;
		}
		wchar_t tmp[MAX_PATH] = { }, dir[MAX_PATH] = { };
		GetTempPathW(MAX_PATH, tmp);
		std::wstring fn = tag;
		SafeFileName(fn);
		swprintf_s(dir, L"%lsZenWare_%ls.exe", tmp, fn.c_str());
		HANDLE hf = CreateFileW(dir, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hf == INVALID_HANDLE_VALUE) return 0;
		DWORD wr = 0;
		WriteFile(hf, exe.data(), (DWORD)exe.size(), &wr, nullptr);
		CloseHandle(hf);
		if (wr != exe.size()) { DeleteFileW(dir); return 0; }
		LoaderUtil::Log(hMain, "[updater] saved, launching");
		LoaderUtil::Status(hMain, LoaderUtil::S("Обновление готово, запуск...", "Update ready, launching..."));
		ShellExecuteW(nullptr, L"open", dir, nullptr, nullptr, SW_SHOWNORMAL);
		Sleep(1500);
		if (hMain && IsWindow(hMain)) PostMessageW(hMain, WM_CLOSE, 0, 0);
		return 0;
	}
}

// __try нельзя в функции с объектами (вектора/строки) — тело отдельно, SEH снаружи.
DWORD WINAPI ThreadProc(LPVOID p) {
	__try {
		return ThreadBody((HWND)p);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return 1;
	}
}

void Updater::CheckAsync(HWND hMain) {
	HANDLE h = CreateThread(nullptr, 0, ThreadProc, hMain, 0, nullptr);
	if (h) CloseHandle(h);
}
