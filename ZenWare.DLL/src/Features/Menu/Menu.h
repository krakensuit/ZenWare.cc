#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Menu
{
public:
	struct MouseState_t
	{
		POINT pt = { };
		bool bDown = false;
		bool bClicked = false;
	};

	//Called from EngineVGui::Paint (inside StartDrawing/FinishDrawing).
	void Render();

	//Called from the WndProc hook on keydown AND from Render as fallback.
	//Single shared edge detector, so one press never toggles twice.
	void PollMenuKey();

	void Toggle();
	void SetOpen(bool bOpen);

	//Called from the WndProc hook. Returns true to swallow the message
	//while the menu is open (blocks game mouse/keyboard input).
	bool ShouldBlockInput(unsigned int uMsg);

private:
	struct Layout_t
	{
		int nX = 0;
		int nY = 0;
		int nW = 0;
		int nH = 0;
	};

	bool HandleOpenState();

	void DrawPanel();
	void Tabs(const MouseState_t& mouse, int& nTab);
	void Checkbox(const MouseState_t& mouse, const char* const szLabel, bool* pValue);
	void Button(const MouseState_t& mouse, const char* const szLabel, void(*pfnAction)());
	void BindRow(const MouseState_t& mouse, const char* const szLabel, int* pValue);
	void LabelInt(const char* const szLabel, const int nValue, int nRightPad = 0);
	void SliderInt(const MouseState_t& mouse, const char* const szLabel, int* pValue, const int nMin, const int nMax);
	bool HelpIcon(const MouseState_t& mouse, const char* const szId, const char* const szTitle, const char* const szText, int nRowX, int nRowW, int nRowY, int nRowH);
	bool HelpMark(const MouseState_t& mouse, const char* const szId, const char* const szTitle, const char* const szText, int nX, int nY);
	void DrawHelpPopup(const MouseState_t& mouse);
	int WrapHelpText(const char* szText, char aLines[][64], int nMaxLines, int nMaxChars);

	static constexpr int PANEL_W = 320;
	static constexpr int PANEL_H = 480;

	Layout_t m_rc = { };
	int m_nItemY = 0;

	int m_nPosX = 0;
	int m_nPosY = 0;
	bool m_bPosInit = false;

	int m_nTab = 0;
	bool m_bDragging = false;
	int m_nDragOffX = 0;
	int m_nDragOffY = 0;

	const char* m_szHelpId = nullptr;
	const char* m_szHelpTitle = nullptr;
	const char* m_szHelpText = nullptr;
};

namespace F { inline CFeatures_Menu Menu; }
