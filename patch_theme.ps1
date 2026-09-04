$ErrorActionPreference = "Stop"
$m = Get-Content "C:\Users\ilya\Desktop\ZenWare.cc\ZenWare.Loader\Main.cpp" -Raw

# 1) uxtheme include
$m = $m -replace '#include <dwmapi\.h>', "#include <dwmapi.h>`n#include <uxtheme.h>"

# 2) remove direct SetWindowTheme calls from WM_CREATE
$m = $m -replace '\r?\n\t\t\t//Dark scrollbars/borders for the edits on Windows 10 1809\+\.\r?\n\t\t\tSetWindowTheme\(g_hwndPath, L"DarkMode_Explorer", nullptr\);\r?\n\t\t\tSetWindowTheme\(g_hwndLog, L"DarkMode_Explorer", nullptr\);', ''

# 3) insert ApplyControlThemes() before AppendLogText
$fn = @"

void ApplyControlThemes()
{
	if (!g_hwndPath || !g_hwndLog)
		return;

	//Dark scrollbars/borders for the edits on Windows 10 1809+.
	LPCWSTR wszClass = g_theme.bDark ? L"DarkMode_Explorer" : nullptr;
	SetWindowTheme(g_hwndPath, wszClass, nullptr);
	SetWindowTheme(g_hwndLog, wszClass, nullptr);
}
"@
$fn2 = $fn -replace "`r`n", "`n"
$idx = $m.IndexOf("	void AppendLogText")
if ($idx -lt 0) { throw "no insert point" }
$m = $m.Substring(0, $idx) + $fn2 + "`n" + $m.Substring($idx)

# 4) call it at the end of RefreshTheme
$m = $m -replace '(\t\tif \(g_hwndMain\)\r?\n\t\t\tInvalidateRect\(g_hwndMain, nullptr, TRUE\);\r?\n\t\})', "`$1`n`n`t`tApplyControlThemes();"

# 5) class brush -> nullptr (we paint the background ourselves)
$m = $m -replace 'reinterpret_cast<HBRUSH>\(COLOR_WINDOW \+ 1\)', 'nullptr'

Set-Content "C:\Users\ilya\Desktop\ZenWare.cc\ZenWare.Loader\Main.cpp" -Value $m -NoNewline
Write-Host "patched"
