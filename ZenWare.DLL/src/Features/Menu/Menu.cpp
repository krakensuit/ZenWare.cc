#include "Menu.h"
#include "../Vars.h"
#include "../Config/Config.h"
#include "../Chams/Chams.h"
#include "../../Entry/Entry.h"
#include "../../Hooks/WndProc/WndProc.h"
#include "../../Util/Anim/Anim.h"
#include "../../external/Icons/IconsFontAwesome6.h"
#include <cmath>
#include <map>
#include <string>
static const Color CLR_SHADOW(0,0,0,80);
static const Color CLR_BG(14,16,15,248);
static const Color CLR_HEADER(18,21,20,255);
static const Color CLR_FOOTER(11,12,12,255);
static const Color CLR_ROW_HOVER(255,255,255,8);
static const Color CLR_ACCENT(0,255,171,255);
static const Color CLR_ACCENT_SOFT(0,255,171,45);
static const Color CLR_TITLE(238,252,247,255);
static const Color CLR_TEXT_ON(235,245,240,255);
static const Color CLR_TEXT_OFF(108,118,113,255);
static const Color CLR_OUTLINE(40,48,44,255);
static const Color CLR_OUTLINE_SOFT(32,38,35,255);
static const char* kStrafeNames[] = { "Legit", "Rage", "W-Only", "Directional" };
static const char* kBhopNames[] = { "Perfect", "Legit" };

namespace {
CFeatures_Menu::MouseState_t GetMouse(){
 static bool s_bPrevDown=false;
 CFeatures_Menu::MouseState_t s; GetCursorPos(&s.pt);
 if(Hooks::WndProc::hwGame) ScreenToClient(Hooks::WndProc::hwGame,&s.pt);
 s.bDown=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
 s.bClicked=(s.bDown && !s_bPrevDown); s_bPrevDown=s.bDown; return s;
}
bool Hovered(const POINT& p,int x,int y,int w,int h){ return p.x>=x&&p.x<=x+w&&p.y>=y&&p.y<=y+h; }
	const char* KeyName(int vk){
		static char b[32]={};
		if(!vk) return "off";
		switch(vk){
			case VK_INSERT: return "INSERT"; case VK_XBUTTON1: return "MOUSE4"; case VK_XBUTTON2: return "MOUSE5";
			case VK_LBUTTON: return "LMB"; case VK_RBUTTON: return "RMB"; case VK_MBUTTON: return "MMB";
			case VK_SPACE: return "SPACE"; case VK_RETURN: return "ENTER"; case VK_ESCAPE: return "ESC";
			case VK_TAB: return "TAB"; case VK_CAPITAL: return "CAPS"; case VK_BACK: return "BACKSPACE";
		}
		UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
		if(sc && GetKeyNameTextA(sc<<16, b, sizeof(b))) return b;
		// extended keys (arrows, etc.) need 0x100 bit
		if(sc && GetKeyNameTextA((sc<<16)|0x1000000, b, sizeof(b))) return b;
		sprintf_s(b,"VK_%d",vk); return b;
	}

	struct HelpEntry_t { const char* label; const char* title; const char* text; };
	static const HelpEntry_t kHelp[] = {
		{"ESP","ESP","Master switch for player ESP. Shows boxes, health and names through walls."},
		{"ESP box","ESP box","2D bounding box around each player, colored by team."},
		{"ESP health bar","ESP health bar","Vertical bar left of the box. Green is full HP, red is low."},
		{"ESP name","ESP name","Player nickname drawn above the box."},
		{"ESP distance","ESP distance","Distance in meters next to the name."},
		{"ESP items","ESP items","Highlights ground weapons, meds and throwables."},
		{"ESP commons","ESP commons","Draws boxes around common infected."},
		{"Snaplines","Snaplines","Line from the bottom of the screen to each player box."},
		{"Chams","Chams","Flat materials on player models, visible through walls."},
		{"Chams through walls","Chams through walls","Ignores depth check so chams show through walls."},
		{"Chams palette >","Chams palette","Cycles 5 enemy and ally color presets."},
		{"No visual recoil","No visual recoil","Removes screen punch locally. Server spread is untouched."},
		{"Bunny hop","Bunny hop","Auto-jump on landing. Hold SPACE while moving."},
		{"Bhop: Perfect >","Bhop style","Perfect forces a jump every tick. Legit only uses your own keypress."},
		{"Bhop: Legit >","Bhop style","Perfect forces a jump every tick. Legit only uses your own keypress."},
		{"Auto strafe","Auto strafe","Automatic air strafing. Pick a style below."},
		{"Strafe: Legit >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D."},
		{"Strafe: Rage >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D."},
		{"Strafe: W-Only >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D."},
		{"Strafe: Directional >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D."},
		{"Bhop delay","Bhop delay","Minimum ticks between jumps. 0 is the fastest."},
		{"Edge jump","Edge jump","Auto-jumps when you walk off a ledge while holding jump."},
		{"Edge bug","Edge bug","Ducks before hard landings to keep your speed."},
		{"Jump bug","Jump bug","Duck-taps landings to negate fall damage. Releases duck on ground."},
		{"Null movement","Null movement","Cancels opposite keys (A+D, W+S) for clean strafes."},
		{"Fast stop","Fast stop","Counter-strafes to a full stop when no keys are held."},
		{"Speed HUD","Speed HUD","Shows current velocity under the crosshair."},
		{"Jump stats","Jump stats","KZ-style panel: distance, prestrafe, max speed, strafes, sync, edge and EB marks."},
		{"Prestrafe","Prestrafe","Forces full forward speed on ground jumps."},
		{"Long jump helper","Long jump helper","Auto-ducks on jump for extra longjump distance."},
		{"Viewmodel FOV x100","Viewmodel FOV","Weapon viewmodel field of view multiplier."},
		{"No fog","No fog","Disables world fog."},
		{"Crosshair","Crosshair","Custom center crosshair."},
		{"Crosshair size","Crosshair size","Crosshair arm length in pixels."},
		{"FPS / pos overlay","FPS overlay","FPS and position readout in the bottom-left corner."},
		{"Aimbot","Aimbot","Silent aim at head or center within FOV. Hold the aim key."},
		{"Auto shoot","Auto shoot","Fires automatically while a target is locked."},
		{"Silent aim","Silent aim","The server sees aimed angles, your screen stays still."},
		{"Target commons","Target commons","Aimbot and triggerbot also lock common infected and the witch, not just specials."},
		{"Aim FOV x10","Aim FOV","Target search radius around the crosshair, in 0.1 degrees."},
		{"Smoothing","Smoothing","0 snaps instantly. Higher values look more human."},
		{"Aimbot key","Aimbot key","Hold to enable the aimbot. Click to rebind, ESC clears."},
		{"Trigger bot","Trigger bot","Shoots when the crosshair is on a visible enemy."},
		{"Trigger key","Trigger key","Hold to enable the triggerbot. Click to rebind."},
		{"Auto pistol","Auto pistol","Re-clicks semi-auto pistols for hold-to-fire."},
		{"Auto shove","Auto shove","Auto-shoves tongue and pounce attackers off teammates."},
		{"Save config","Save config","Writes all settings to ZenWare.cfg."},
		{"Load config","Load config","Reads settings back from ZenWare.cfg."},
		{"Menu key","Menu key","Opens and closes this menu. Click to rebind."},
	};
	static const HelpEntry_t* FindHelp(const char* szLabel){
		for(size_t i=0;i<sizeof(kHelp)/sizeof(kHelp[0]);i++)
			if(!strcmp(kHelp[i].label,szLabel)) return &kHelp[i];
		return nullptr;
	}
}
void CFeatures_Menu::Toggle(){
 SetOpen(!Vars::Menu::bOpen);
}
void CFeatures_Menu::SetOpen(bool bOpen){
 if(Vars::Menu::bOpen==bOpen) return;
 Vars::Menu::bOpen=bOpen;
 if(bOpen){ while(ShowCursor(TRUE) < 0); } else { while(ShowCursor(FALSE) >= 0); }
 auto lock = [&](bool bLock){
  if(I::VGuiSurface) bLock?I::VGuiSurface->LockCursor():I::VGuiSurface->UnlockCursor();
  if(I::MatSystemSurface) bLock?I::MatSystemSurface->LockCursor():I::MatSystemSurface->UnlockCursor();
 };
 lock(!bOpen);
}

Color HsvToColor(float h, float s, float v){
 while(h<0.0f) h+=360.0f; while(h>=360.0f) h-=360.0f;
 const float c=v*s;
 const float x=c*(1.0f-fabsf(fmodf(h/60.0f,2.0f)-1.0f));
 const float m=v-c;
 float r=0.0f,g=0.0f,b=0.0f;
 if(h<60.0f){ r=c; g=x; } else if(h<120.0f){ r=x; g=c; } else if(h<180.0f){ g=c; b=x; }
 else if(h<240.0f){ g=x; b=c; } else if(h<300.0f){ r=x; b=c; } else { r=c; b=x; }
 return Color((int)((r+m)*255.0f),(int)((g+m)*255.0f),(int)((b+m)*255.0f),255);
}

void DrawRgbLogo(int x, int y){
 static const char* txt="ZenWare.cc";
 const float hue=fmodf((float)GetTickCount64()/38.0f,360.0f);
 for(int dx=-2;dx<=2;dx+=2) for(int dy=-2;dy<=2;dy+=2){
  if(!dx&&!dy) continue;
  G::Draw.String(EFonts::MENU_TAB,x+dx,y+dy,HsvToColor(hue,0.9f,0.35f),TXT_DEFAULT,"%s",txt);
 }
 int cx=x;
 for(const char* p=txt;*p;++p){
  const int idx=(int)(p-txt);
  char ch[2]={*p,0};
  G::Draw.String(EFonts::MENU_TAB,cx,y,HsvToColor(hue+idx*5.0f,0.85f,1.0f),TXT_DEFAULT,"%s",ch);
  cx+=G::Draw.GetTextWidth(EFonts::MENU_TAB,ch);
 }
}
void CFeatures_Menu::PollMenuKey(){
 if(Vars::Menu::nKey==0) return;
 static bool s_bPrev=false;
 const bool bDown=(GetAsyncKeyState(Vars::Menu::nKey)&0x8000)!=0;
 if(bDown && !s_bPrev) Toggle();
 s_bPrev=bDown;
}
bool CFeatures_Menu::HandleOpenState(){
 PollMenuKey();
 return Vars::Menu::bOpen;
}
void CFeatures_Menu::Render(){
 // анимация появления (fade-in) - не блокирует логику открытия
 static float s_alpha = 0.0f;
 bool bOpen = HandleOpenState();
 float dt = I::GlobalVars ? I::GlobalVars->frametime : 0.016f;
 if(dt <= 0 || dt > 0.1f) dt = 0.016f;
 m_flDt = dt;
 s_alpha = Anim::Approach(s_alpha, bOpen ? 1.0f : 0.0f, dt, 9.0f);
 if(s_alpha < 0.01f){
  static bool s_p=false; const bool bF11=(GetAsyncKeyState(VK_F11)&0x8000)!=0; if(bF11&&!s_p) G::ModuleEntry.RequestUnload(); s_p=bF11;
  m_szHelpId=nullptr; m_szHelpTitle=nullptr; m_szHelpText=nullptr;
  if(!Vars::Menu::bOpen && G::Draw.m_nScreenW > 0){
   const float whue=fmodf((float)GetTickCount64()/38.0f,360.0f);
   G::Draw.String(EFonts::MENU_TAHOMA,G::Draw.m_nScreenW-118,G::Draw.m_nScreenH-26,HsvToColor(whue,0.7f,1.0f),TXT_DEFAULT,"ZenWare.cc");
  }
  return;
 }
 if(!G::Draw.m_nScreenW||!G::Draw.m_nScreenH||!Hooks::WndProc::hwGame) return;
 if(I::VGuiSurface) I::VGuiSurface->UnlockCursor();
 if(I::MatSystemSurface) I::MatSystemSurface->UnlockCursor();
 while(ShowCursor(FALSE) >= 0);
 const MouseState_t mouse=GetMouse();
 constexpr int HEADER_H=46, FOOTER_H=22;
 if(!m_bPosInit){ m_nPosX=(G::Draw.m_nScreenW-PANEL_W)/2; m_nPosY=(G::Draw.m_nScreenH-PANEL_H)/3; m_bPosInit=true; }
 if(mouse.bDown && !m_bDragging && Hovered(mouse.pt,m_nPosX,m_nPosY,PANEL_W,HEADER_H)){ m_bDragging=true; m_nDragOffX=mouse.pt.x-m_nPosX; m_nDragOffY=mouse.pt.y-m_nPosY; }
 if(!mouse.bDown) m_bDragging=false;
 if(m_bDragging){ m_nPosX=mouse.pt.x-m_nDragOffX; m_nPosY=mouse.pt.y-m_nDragOffY; }
 Vars::Aimbot::flFOV=Vars::Aimbot::nFOVSlider/10.0f; Vars::Aimbot::flSmoothing=(float)Vars::Aimbot::nSmoothSlider; Vars::Visuals::flViewFOV=Vars::Visuals::nViewFOVSlider/100.0f;
 m_rc.nX=m_nPosX; m_rc.nY=m_nPosY-(int)((1.0f-s_alpha)*16); m_rc.nW=PANEL_W; m_rc.nH=PANEL_H;
 G::Draw.Rect(m_rc.nX+5,m_rc.nY+6,m_rc.nW,m_rc.nH,CLR_SHADOW);
 G::Draw.Rect(m_rc.nX+3,m_rc.nY+4,m_rc.nW,m_rc.nH,CLR_SHADOW);
 G::Draw.Rect(m_rc.nX+1,m_rc.nY+2,m_rc.nW,m_rc.nH,CLR_SHADOW);
 DrawPanel(); Tabs(mouse,m_nTab);
 m_nItemY=m_rc.nY+HEADER_H+34;
 switch(m_nTab){
  case 0:{
   Checkbox(mouse,"ESP",&Vars::ESP::bEnabled);
   Checkbox(mouse,"ESP box",&Vars::ESP::bBox);
   Checkbox(mouse,"ESP health bar",&Vars::ESP::bHealthBar);
   Checkbox(mouse,"ESP name",&Vars::ESP::bName);
   Checkbox(mouse,"ESP distance",&Vars::ESP::bDistance);
   Checkbox(mouse,"ESP items",&Vars::ESP::bItems);
    Checkbox(mouse,"ESP commons",&Vars::ESP::bCommon);
    Checkbox(mouse,"Snaplines",&Vars::ESP::bSnaplines);
   Checkbox(mouse,"Chams",&Vars::Chams::bEnabled);
   Checkbox(mouse,"Chams through walls",&Vars::Chams::bThroughWalls);
   Button(mouse,"Chams palette >",[](){ Vars::Chams::nPalette=(Vars::Chams::nPalette+1)%5; });
   Checkbox(mouse,"No visual recoil",&Vars::VisualRecoil::bEnabled);
   break;
  }
   case 1:{
    Checkbox(mouse,"Bunny hop",&Vars::BunnyHop::bEnabled);
    static char szBhopStyle[32]; sprintf_s(szBhopStyle,"Bhop: %s >",kBhopNames[U::Math.Clamp(Vars::BunnyHop::nBhopStyle,0,1)]);
    Button(mouse,szBhopStyle,[](){ Vars::BunnyHop::nBhopStyle=(Vars::BunnyHop::nBhopStyle+1)%2; });
    Checkbox(mouse,"Auto strafe",&Vars::BunnyHop::bAutoStrafe);
    static char szStrafe[32]; sprintf_s(szStrafe,"Strafe: %s >",kStrafeNames[U::Math.Clamp(Vars::BunnyHop::nAutoStrafeMode,0,3)]);
    Button(mouse,szStrafe,[](){ Vars::BunnyHop::nAutoStrafeMode=(Vars::BunnyHop::nAutoStrafeMode+1)%4; });
    SliderInt(mouse,"Bhop delay",&Vars::BunnyHop::nJumpDelayTicks,0,20);
    Checkbox(mouse,"Edge jump",&Vars::BunnyHop::bEdgeJump);
    Checkbox(mouse,"Edge bug",&Vars::BunnyHop::bEdgeBug);
    Checkbox(mouse,"Jump bug",&Vars::BunnyHop::bJumpBug);
    Checkbox(mouse,"Null movement",&Vars::BunnyHop::bNullMove);
    Checkbox(mouse,"Fast stop",&Vars::BunnyHop::bFastStop);
    Checkbox(mouse,"Speed HUD",&Vars::BunnyHop::bSpeedHUD);
    Checkbox(mouse,"Jump stats",&Vars::BunnyHop::bJumpStats);
    Checkbox(mouse,"Prestrafe",&Vars::BunnyHop::bPrestrafe);
    Checkbox(mouse,"Long jump helper",&Vars::BunnyHop::bLongJumpHelper);
    break;
   }
   case 2:{
    SliderInt(mouse,"Viewmodel FOV x100",&Vars::Visuals::nViewFOVSlider,50,300);
    Checkbox(mouse,"No fog",&Vars::Visuals::bNoFog);
    Checkbox(mouse,"Crosshair",&Vars::Visuals::bCrosshair);
    SliderInt(mouse,"Crosshair size",&Vars::Visuals::nCrosshairSize,2,30);
    Checkbox(mouse,"FPS / pos overlay",&Vars::Visuals::bOverlay);
    break;
   }
   case 3:{
   Checkbox(mouse,"Aimbot",&Vars::Aimbot::bEnabled);
   Checkbox(mouse,"Auto shoot",&Vars::Aimbot::bAutoShoot);
    Checkbox(mouse,"Silent aim",&Vars::Aimbot::bSilent);
    Checkbox(mouse,"Target commons",&Vars::Aimbot::bTargetCommons);
   SliderInt(mouse,"Aim FOV x10",&Vars::Aimbot::nFOVSlider,5,300);
   SliderInt(mouse,"Smoothing",&Vars::Aimbot::nSmoothSlider,0,60);
   BindRow(mouse,"Aimbot key",&Vars::Aimbot::nKey);
   Checkbox(mouse,"Trigger bot",&Vars::TriggerBot::bEnabled);
   BindRow(mouse,"Trigger key",&Vars::TriggerBot::nKey);
   Checkbox(mouse,"Auto pistol",&Vars::AutoPistol::bEnabled);
   Checkbox(mouse,"Auto shove",&Vars::AutoShove::bEnabled);
   break;
  }
  default:{
   Button(mouse,"Save config",[](){F::Config.Save();});
   Button(mouse,"Load config",[](){F::Config.Load();});
   BindRow(mouse,"Menu key",&Vars::Menu::nKey);
   G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+20,m_nItemY+6,CLR_TEXT_OFF,TXT_DEFAULT,"F11 = unload cheat");
   m_nItemY+=26; break;
  }
 }
 const float fhue=fmodf((float)GetTickCount64()/38.0f,360.0f);
 G::Draw.GradientRect(m_rc.nX+1,(m_rc.nY+m_rc.nH)-FOOTER_H-2,m_rc.nX+m_rc.nW-1,(m_rc.nY+m_rc.nH)-FOOTER_H-1,HsvToColor(fhue,0.85f,1.0f),HsvToColor(fhue+140.0f,0.85f,1.0f),true);
 G::Draw.Rect(m_rc.nX+1,(m_rc.nY+m_rc.nH)-FOOTER_H-1,m_rc.nW-2,FOOTER_H,CLR_FOOTER);
 G::Draw.String(EFonts::MENU_CONSOLAS,m_rc.nX+(m_rc.nW/2),(m_rc.nY+m_rc.nH)-FOOTER_H+4,CLR_TEXT_OFF,TXT_CENTERXY,"drag header | WASD free | F11 unload | %d fps",(int)(1.0f/m_flDt));
 G::Draw.OutlinedRect(m_rc.nX,m_rc.nY,m_rc.nW,m_rc.nH,CLR_OUTLINE);
 {
  const float ehue=fmodf((float)GetTickCount64()/38.0f,360.0f);
  const int epulse=25+(int)(20*sinf((GetTickCount64()%6283)/1000.0f));
  Color edge=HsvToColor(ehue,0.9f,0.55f);
  edge.SetColor(edge.r(),edge.g(),edge.b(),epulse);
  G::Draw.OutlinedRect(m_rc.nX+1,m_rc.nY+1,m_rc.nW-2,m_rc.nH-2,edge);
 }
 DrawHelpPopup(mouse);
 //custom crosshair cursor (OS cursor stays hidden while the menu is open)
 G::Draw.Line(mouse.pt.x-7,mouse.pt.y,mouse.pt.x-2,mouse.pt.y,CLR_ACCENT);
 G::Draw.Line(mouse.pt.x+2,mouse.pt.y,mouse.pt.x+7,mouse.pt.y,CLR_ACCENT);
 G::Draw.Line(mouse.pt.x,mouse.pt.y-7,mouse.pt.x,mouse.pt.y-2,CLR_ACCENT);
 G::Draw.Line(mouse.pt.x,mouse.pt.y+2,mouse.pt.x,mouse.pt.y+7,CLR_ACCENT);
 G::Draw.Circle(mouse.pt.x,mouse.pt.y,1,8,CLR_ACCENT);
}
bool CFeatures_Menu::ShouldBlockInput(unsigned int uMsg){
 if(!Vars::Menu::bOpen) return false;
 switch(uMsg){case WM_LBUTTONDOWN:case WM_LBUTTONUP:case WM_RBUTTONDOWN:case WM_RBUTTONUP:case WM_MBUTTONDOWN:case WM_MBUTTONUP:case WM_MOUSEWHEEL:return true; default:return false;}
}
bool CFeatures_Menu::HelpMark(const MouseState_t& mouse,const char* const szId,const char* const szTitle,const char* const szText,int nX,int nY){
 constexpr int D=13;
 const int nCX=nX+D/2, nCY=nY+D/2;
 const bool bOpen=(m_szHelpId&&szId&&!strcmp(m_szHelpId,szId));
 const bool bHov=Hovered(mouse.pt,nX-2,nY-2,D+4,D+4);
 if(bOpen) G::Draw.Circle(nCX,nCY,6,14,CLR_ACCENT);
 else if(bHov) G::Draw.Circle(nCX,nCY,6,14,CLR_ACCENT_SOFT);
 G::Draw.OutlinedCircle(nCX,nCY,6,14,(bHov||bOpen)?CLR_ACCENT:CLR_TEXT_OFF);
 G::Draw.String(EFonts::MENU_TAHOMA,nCX,nCY,bOpen?Color(8,14,11,255):(bHov?CLR_ACCENT:CLR_TEXT_OFF),TXT_CENTERXY,"?");
 const bool bClick=(bHov&&mouse.bClicked);
 if(bClick){
  if(bOpen){ m_szHelpId=nullptr; m_szHelpTitle=nullptr; m_szHelpText=nullptr; }
  else { m_szHelpId=szId; m_szHelpTitle=szTitle; m_szHelpText=szText; }
 }
 return bClick;
}
bool CFeatures_Menu::HelpIcon(const MouseState_t& mouse,const char* const szId,const char* const szTitle,const char* const szText,int nRowX,int nRowW,int nRowY,int nRowH){
 constexpr int D=13;
 return HelpMark(mouse,szId,szTitle,szText,nRowX+nRowW-8-D,nRowY+(nRowH-D)/2);
}
int CFeatures_Menu::WrapHelpText(const char* szText,char aLines[][64],int nMaxLines,int nMaxChars){
 int nLines=0;
 if(!szText||!szText[0]||nMaxLines<=0||nMaxChars<=0) return 0;
 char cur[64]={}; int nCur=0;
 const char* p=szText;
 while(*p&&nLines<nMaxLines){
  while(*p==' ') p++;
  if(!*p) break;
  const char* w=p; while(*p&&*p!=' ') p++;
  int nW=(int)(p-w); if(nW>=64) nW=63;
  if(nCur>0&&nCur+1+nW>nMaxChars){
   strcpy_s(aLines[nLines],cur); nLines++; if(nLines>=nMaxLines) return nLines;
   nCur=0; cur[0]='\0';
  }
  if(nCur>0){ strcat_s(cur," "); nCur++; }
  strncat_s(cur,w,nW); nCur+=nW;
 }
 if(nCur>0&&nLines<nMaxLines){ strcpy_s(aLines[nLines],cur); nLines++; }
 return nLines;
}
void CFeatures_Menu::DrawHelpPopup(const MouseState_t& mouse){
 (void)mouse;
 if(!m_szHelpText||!m_szHelpTitle) return;
 char aLines[12][64]={};
 const int n=WrapHelpText(m_szHelpText,aLines,12,44);
 if(n<=0) return;
 const int nLineH=G::Draw.GetFontHeight(EFonts::MENU_TAHOMA)+3;
 const int nTitleH=G::Draw.GetFontHeight(EFonts::MENU_TAHOMA)+10;
 int nW=G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,m_szHelpTitle)+28;
 for(int i=0;i<n;i++){ const int w=G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,aLines[i])+28; if(w>nW) nW=w; }
 if(nW>320) nW=320;
 const int nH=nTitleH+n*nLineH+12;
 int nX=m_rc.nX+m_rc.nW+8, nY=m_rc.nY+m_rc.nH-nH-28;
 if(nX+nW>G::Draw.m_nScreenW-8) nX=m_rc.nX-nW-8;
 if(nX<8) nX=8;
 if(nY<8) nY=8;
 if(nY+nH>G::Draw.m_nScreenH-8) nY=G::Draw.m_nScreenH-8-nH;
 G::Draw.Rect(nX+3,nY+4,nW,nH,CLR_SHADOW);
 G::Draw.Rect(nX,nY,nW,nH,CLR_BG);
 G::Draw.Rect(nX,nY,nW,2,CLR_ACCENT);
 G::Draw.String(EFonts::MENU_TAHOMA,nX+14,nY+5,CLR_ACCENT,TXT_DEFAULT,"%s",m_szHelpTitle);
 G::Draw.OutlinedRect(nX,nY,nW,nH,CLR_OUTLINE);
 for(int i=0;i<n;i++)
  G::Draw.String(EFonts::MENU_TAHOMA,nX+14,nY+nTitleH+i*nLineH,CLR_TEXT_ON,TXT_DEFAULT,"%s",aLines[i]);
}
void CFeatures_Menu::DrawPanel(){
 G::Draw.Rect(m_rc.nX,m_rc.nY,m_rc.nW,m_rc.nH,CLR_BG);
 G::Draw.Rect(m_rc.nX,m_rc.nY,2,m_rc.nH,CLR_ACCENT_SOFT);
 G::Draw.Rect(m_rc.nX+2,m_rc.nY,m_rc.nW-2,40,CLR_HEADER);
	DrawRgbLogo(m_rc.nX+16,m_rc.nY+4);
 G::Draw.GradientRect(m_rc.nX+2,m_rc.nY+40,m_rc.nX+m_rc.nW,m_rc.nY+43,CLR_ACCENT,CLR_ACCENT_SOFT,false);
 {
  float shx = fmodf((float)GetTickCount64() / 12.0f, (float)(m_rc.nW + 120)) - 60;
  G::Draw.Rect(m_rc.nX + 2 + (int)shx, m_rc.nY, 60, 40, Color(255,255,255,10));
 }
}
void CFeatures_Menu::Tabs(const MouseState_t& mouse,int& nTab){
 const char* szTabs[]={"Visuals","Move","View","Combat","Misc"};
 const int nTabW=(m_rc.nW-20)/5;
 static float s_pill=0.0f;
 s_pill=Anim::Approach(s_pill,(float)m_nTab,m_flDt,10.0f);
 const int nTabY=m_rc.nY+52; constexpr int nTabH=22;
 const int nPillX=m_rc.nX+10+(int)(s_pill*(float)nTabW);
 G::Draw.Rect(nPillX,nTabY,nTabW-6,nTabH,CLR_ACCENT);
 G::Draw.Rect(nPillX,nTabY+nTabH-2,nTabW-6,2,CLR_ACCENT_SOFT);
 for(int n=0;n<5;n++){
  int nX=m_rc.nX+10+(n*nTabW); int nY=m_rc.nY+52; constexpr int nH=22;
  bool bActive=(m_nTab==n); bool bHover=Hovered(mouse.pt,nX,nY,nTabW-6,nH);
  Color clrText=bActive?Color(8,14,11,255):(bHover?CLR_TEXT_ON:CLR_TEXT_OFF);
  if(!bActive&&bHover){ G::Draw.Rect(nX,nY,nTabW-6,nH,CLR_ROW_HOVER); G::Draw.Rect(nX,nY+nH-2,nTabW-6,2,CLR_ACCENT_SOFT); }
  G::Draw.String(EFonts::MENU_TAHOMA,nX+((nTabW-6)/2),nY+4,clrText,TXT_CENTERXY,szTabs[n]);
  if(bHover&&mouse.bClicked) m_nTab=n;
 } nTab=m_nTab;
}
static Color LerpC(const Color& a,const Color& b,float t){
 int ar,ag,ab,aa,br,bg2,bb2,ba;
 a.GetColor(ar,ag,ab,aa); b.GetColor(br,bg2,bb2,ba);
 if(t<0) t=0; if(t>1) t=1;
 return Color(ar+(int)((br-ar)*t),ag+(int)((bg2-ag)*t),ab+(int)((bb2-ab)*t),255);
}
static std::map<std::string,float> s_tog;
void CFeatures_Menu::Checkbox(const MouseState_t& mouse,const char* szLabel,bool* pValue){
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=24;
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 if(bHover) G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,CLR_ROW_HOVER);
 if(bHover) G::Draw.Rect(nRowX,m_nItemY,2,nRowH,CLR_ACCENT);
 // toggle track 32x16
 constexpr int nTogW=30, nTogH=14;
 int nTogX=nRowX+nRowW-nTogW-10;
 int nTogY=m_nItemY+(nRowH-nTogH)/2;
 float &flTog=s_tog[szLabel];
 flTog=Anim::Approach(flTog,*pValue?1.0f:0.0f,m_flDt,12.0f);
 Color trackClr=LerpC(CLR_OUTLINE,CLR_ACCENT,flTog);
 G::Draw.Rect(nTogX,nTogY,nTogW,nTogH,trackClr);
 G::Draw.OutlinedRect(nTogX,nTogY,nTogW,nTogH,LerpC(CLR_OUTLINE_SOFT,CLR_ACCENT,flTog));
 // knob
 int nKnobX=nTogX+7+(int)((nTogW-14)*flTog);
 G::Draw.Circle(nKnobX,nTogY+nTogH/2,5,16,Color(245,255,250,255));
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+12+(bHover?2:0),m_nItemY+6,*pValue?CLR_TEXT_ON:CLR_TEXT_OFF,TXT_DEFAULT,szLabel);
 bool bHelp=false;
 if(const HelpEntry_t* he=FindHelp(szLabel)){
  const int nQX=nRowX+12+G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,szLabel)+7;
  bHelp=HelpMark(mouse,he->label,he->title,he->text,nQX,m_nItemY+5);
 }
 if(bHover&&mouse.bClicked&&!bHelp) *pValue=!(*pValue);
 m_nItemY+=nRowH;
}
void CFeatures_Menu::Button(const MouseState_t& mouse,const char* szLabel,void(*pfnAction)()){
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=26;
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 if(bHover) G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,CLR_ROW_HOVER);
 if(bHover) G::Draw.Rect(nRowX,m_nItemY,2,nRowH,CLR_ACCENT);
 G::Draw.Rect(nRowX+8,m_nItemY+3,96,nRowH-7,CLR_HEADER);
 G::Draw.OutlinedRect(nRowX+8,m_nItemY+3,96,nRowH-7,bHover?CLR_ACCENT:CLR_OUTLINE);
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+8+48,m_nItemY+7,bHover?CLR_ACCENT:CLR_TEXT_ON,TXT_CENTERXY,szLabel);
 bool bHelp=false;
 if(const HelpEntry_t* he=FindHelp(szLabel))
  bHelp=HelpIcon(mouse,he->label,he->title,he->text,nRowX,nRowW,m_nItemY,nRowH);
 if(bHover&&mouse.bClicked&&!bHelp&&pfnAction) pfnAction();
 m_nItemY+=nRowH;
}
void CFeatures_Menu::BindRow(const MouseState_t& mouse,const char* szLabel,int* pValue){
 static int* s_pCapturing=nullptr;
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=24;
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 if(bHover) G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,CLR_ROW_HOVER);
 if(bHover) G::Draw.Rect(nRowX,m_nItemY,2,nRowH,CLR_ACCENT);
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+12+(bHover?2:0),m_nItemY+6,CLR_TEXT_ON,TXT_DEFAULT,szLabel);
 char szVal[32]={};
 if(s_pCapturing==pValue){
  strcpy_s(szVal,"[press key]");
  for(int vk=0x08;vk<0xFE;vk++){ if(vk==VK_LBUTTON||vk==VK_RBUTTON||vk==VK_MBUTTON) continue; if(GetAsyncKeyState(vk)&0x8000){ if(vk==VK_ESCAPE) *pValue=0; else *pValue=vk; s_pCapturing=nullptr; break; } }
 } else { strcpy_s(szVal,"["); strcat_s(szVal,KeyName(*pValue)); strcat_s(szVal,"]"); }
 Color clrVal=(s_pCapturing==pValue)?CLR_ACCENT:CLR_TEXT_ON;
 if(s_pCapturing==pValue){ int pp=150+(int)(105*sinf((GetTickCount64()%6283)/1000.0f)); clrVal=Color(0,255,171,pp); }
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+nRowW-90,m_nItemY+6,clrVal,TXT_DEFAULT,szVal);
 bool bHelp=false;
 if(const HelpEntry_t* he=FindHelp(szLabel)){
  const int nQX=nRowX+12+G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,szLabel)+7;
  bHelp=HelpMark(mouse,he->label,he->title,he->text,nQX,m_nItemY+5);
 }
 if(bHover&&mouse.bClicked&&s_pCapturing!=pValue&&!bHelp) s_pCapturing=pValue;
 m_nItemY+=nRowH;
}
void CFeatures_Menu::LabelInt(const char* szLabel,const int nValue,int nRightPad){
 G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+20,m_nItemY,CLR_TEXT_ON,TXT_DEFAULT,"%s",szLabel);
 char szVal[16]={}; sprintf_s(szVal,"%i",nValue);
 G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+m_rc.nW-44-nRightPad,m_nItemY,CLR_ACCENT,TXT_DEFAULT,szVal);
 m_nItemY+=G::Draw.GetFontHeight(EFonts::MENU_TAHOMA)+5;
}
void CFeatures_Menu::SliderInt(const MouseState_t& mouse,const char* szLabel,int* pValue,const int nMin,const int nMax){
 const int nLblY=m_nItemY;
 LabelInt(szLabel,*pValue);
 if(const HelpEntry_t* he=FindHelp(szLabel)){
  const int nQX=m_rc.nX+20+G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,szLabel)+7;
  HelpMark(mouse,he->label,he->title,he->text,nQX,nLblY-1);
 }
 int nX=m_rc.nX+20, nW=m_rc.nW-40; constexpr int nTrackH=4, nKnobR=5;
 float flFrac=((*pValue-nMin)/static_cast<float>(nMax-nMin)); int nFillW=int(nW*flFrac);
 G::Draw.Rect(nX,m_nItemY,nW,nTrackH,CLR_HEADER);
 G::Draw.Rect(nX,m_nItemY,nFillW,nTrackH,CLR_ACCENT);
 if(nFillW>2) G::Draw.Rect(nX+nFillW-6,m_nItemY,6,nTrackH,Color(200,255,240,255));
 G::Draw.Circle(nX+nFillW,m_nItemY+nTrackH/2,nKnobR+4,14,Color(0,255,171,35));
 G::Draw.Circle(nX+nFillW,m_nItemY+nTrackH/2,nKnobR,14,CLR_ACCENT);
 if(mouse.bDown&&Hovered(mouse.pt,nX-8,m_nItemY-8,nW+16,nTrackH+16)){
  float flNew=U::Math.Clamp((mouse.pt.x-nX)/static_cast<float>(nW),0.0f,1.0f);
  *pValue=nMin+int(flNew*(nMax-nMin));
 }
 m_nItemY+=20;
}
