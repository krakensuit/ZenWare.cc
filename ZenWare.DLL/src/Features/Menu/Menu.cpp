#include "Menu.h"
#include "../Vars.h"
#include "../Lang/Lang.h"
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
static Color CLR_ACCENT(0,255,171,255);
static Color CLR_ACCENT_SOFT(0,255,171,45);
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

	struct HelpEntry_t { const char* label; const char* title; const char* text; const char* ruTitle; const char* ruText; };
	static const HelpEntry_t kHelp[] = {
		{"ESP","ESP","Master switch for player ESP. Shows boxes, health and names through walls.","ESP","Главный выключатель ESP. Боксы, HP и ники сквозь стены."},
		{"ESP box","ESP box","2D bounding box around each player, colored by team.","ESP бокс","2D-бокс вокруг игрока, цвет по команде."},
		{"ESP health bar","ESP health bar","Vertical bar left of the box. Green is full HP, red is low.","ESP полоса HP","Вертикальная полоса слева от бокса. Зелёная — полное HP, красная — низкое."},
		{"ESP name","ESP name","Player nickname drawn above the box.","ESP ники","Ник игрока над боксом."},
		{"ESP distance","ESP distance","Distance in meters next to the name.","ESP дистанция","Дистанция в метрах рядом с ником."},
		{"ESP items","ESP items","Highlights ground weapons, meds and throwables.","ESP предметы","Подсветка оружия, аптечек и гранат на земле."},
		{"ESP commons","ESP commons","Draws boxes around common infected.","ESP обычные","Боксы вокруг обычных заражённых."},
		{"ESP special infected","ESP special infected","Boxes and names for hunter/smoker/jockey/spitter/charger/tank.","ESP особые","Боксы и имена охотника, курильщика, жокея, плевальщицы, громилы и танка."},
		{"ESP witch","ESP witch","Purple box around the witch.","ESP ведьма","Фиолетовый бокс вокруг ведьмы."},
		{"HP text near bar","HP text near bar","Prints the HP number left of the health bar.","HP текст","Число HP слева от полосы здоровья."},
		{"Weapon text","Weapon text","Active weapon name under the nickname.","Текст оружия","Название оружия в руках под ником."},
		{"Hitbox: Head >","Hitbox","Where the aimbot aims: head or body center.","Хитбокс","Куда целится аимбот: голова или центр тела."},
		{"Hitbox: Center >","Hitbox","Where the aimbot aims: head or body center.","Хитбокс","Куда целится аимбот: голова или центр тела."},
		{"Priority: FOV >","Target priority","FOV picks the closest to the crosshair, Distance the nearest player.","Приоритет цели","FOV — ближайший к прицелу, Дистанция — ближайший игрок."},
		{"Priority: Distance >","Target priority","FOV picks the closest to the crosshair, Distance the nearest player.","Приоритет цели","FOV — ближайший к прицелу, Дистанция — ближайший игрок."},
		{"Visible only","Visible only","Only aim at targets with a clear line of sight.","Только видимые","Наведение только на цели на прямой видимости."},
		{"Skip incapped","Skip incapped","Ignore players who are incapacitated.","Без лежачих","Игнорировать лежачих игроков без сознания."},
		{"Trigger visible only","Trigger visible only","Trigger only fires when the target is visible (no walls).","Триггер видим. only","Триггер стреляет только по видимой цели, не сквозь стены."},
		{"Snaplines","Snaplines","Line from the bottom of the screen to each player box.","Снаплайны","Линия от низа экрана к боксу игрока."},
		{"Filled boxes","Filled boxes","Translucent team-colored fill inside ESP boxes.","Заливка боксов","Полупрозрачная заливка боксов цветом команды."},
		{"Chams","Chams","Flat materials on player models, visible through walls.","Чамсы","Плоские материалы на моделях, видно сквозь стены."},
		{"Chams through walls","Chams through walls","Ignores depth check so chams show through walls.","Чамсы сквозь стены","Отключает проверку глубины: чамсы видно сквозь стены."},
		{"Chams palette >","Chams palette","Cycles 5 enemy and ally color presets.","Палитра чамсов","Перебор 5 пресетов цветов врагов и союзников."},
		{"No visual recoil","No visual recoil","Removes screen punch locally. Server spread is untouched.","Без виз. отдачи","Убирает тряску экрана локально. Разброс сервера не трогает."},
		{"Bunny hop","Bunny hop","Auto-jump on landing. Hold SPACE while moving.","Бхоп","Авто-прыжок при приземлении. Держи ПРОБЕЛ в движении."},
		{"Bhop: Perfect >","Bhop style","Perfect forces a jump every tick. Legit only uses your own keypress.","Стиль бхопа","Идеал жмёт прыжок каждый тик. Легит использует только твоё нажатие."},
		{"Bhop: Legit >","Bhop style","Perfect forces a jump every tick. Legit only uses your own keypress.","Стиль бхопа","Идеал жмёт прыжок каждый тик. Легит использует только твоё нажатие."},
		{"Auto strafe","Auto strafe","Automatic air strafing. Pick a style below.","Авто-стрейф","Автоматические стрейфы в воздухе. Стиль выбери ниже."},
		{"Strafe: Legit >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D.","Режим стрейфа","Легит идёт за мышью. Рейдж крутится по кругу. W-Only держит вперёд. Направл. слушает A и D."},
		{"Strafe: Rage >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D.","Режим стрейфа","Легит идёт за мышью. Рейдж крутится по кругу. W-Only держит вперёд. Направл. слушает A и D."},
		{"Strafe: W-Only >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D.","Режим стрейфа","Легит идёт за мышью. Рейдж крутится по кругу. W-Only держит вперёд. Направл. слушает A и D."},
		{"Strafe: Directional >","Strafe mode","Legit follows your mouse. Rage circle-strafes. W-Only keeps forward. Directional respects A and D.","Режим стрейфа","Легит идёт за мышью. Рейдж крутится по кругу. W-Only держит вперёд. Направл. слушает A и D."},
		{"Bhop delay","Bhop delay","Minimum ticks between jumps. 0 is the fastest.","Задержка бхопа","Минимум тиков между прыжками. 0 — самый быстрый."},
		{"Edge jump","Edge jump","Auto-jumps when you walk off a ledge while holding jump.","Эдж-джамп","Авто-прыжок при сходе с края с зажатым прыжком."},
		{"Edge bug","Edge bug","Ducks before hard landings to keep your speed.","Эджбаг","Присед перед жёсткой посадкой, чтобы сохранить скорость."},
		{"Jump bug","Jump bug","Duck-taps landings to negate fall damage. Releases duck on ground.","Джампбаг","Короткий присед в посадке гасит урон от падения. На земле присед отпускается."},
		{"Null movement","Null movement","Cancels opposite keys (A+D, W+S) for clean strafes.","Нулл-мувмент","Гасит противоположные клавиши (A+D, W+S) для чистых стрейфов."},
		{"Fast stop","Fast stop","Counter-strafes to a full stop when no keys are held.","Быстрый стоп","Контр-стрейф до полной остановки, когда клавиши отпущены."},
		{"Speed HUD","Speed HUD","Shows current velocity under the crosshair.","Скорость HUD","Показывает текущую скорость под прицелом."},
		{"Jump stats","Jump stats","KZ-style panel: distance, prestrafe, max speed, strafes, sync, edge and EB marks.","Стата прыжка","Панель в стиле KZ: дистанция, престрейф, макс. скорость, стрейфы, синхрон, метки края и EB."},
		{"Auto duck","Auto duck","Holds duck through the whole airtime for longer jumps and duck-landings.","Авто-присед","Держит присед весь полёт: прыжки дальше, посадки в приседе."},
		{"Prestrafe","Prestrafe","Forces full forward speed on ground jumps.","Престрейф","Форсирует полную скорость вперёд на прыжках с земли."},
		{"Long jump helper","Long jump helper","Auto-ducks on jump for extra longjump distance.","Лонгджамп","Авто-присед в прыжке для extra-дистанции лонга."},
		{"FOV x100 (view+model)","FOV","Field of view multiplier: world camera and viewmodel.","FOV","Множитель обзора: камера мира и модель оружия."},
		{"Third person","Third person","Camera behind the back (local server). Set the distance below.","3-е лицо","Камера за спиной (локальный сервер). Дистанция ниже."},
		{"3rd person distance","3rd person distance","How far the camera sits behind you.","Дистанция камеры","Как далеко камера висит за спиной."},
		{"No fog","No fog","Disables world fog.","Без тумана","Отключает туман мира."},
		{"Crosshair","Crosshair","Custom center crosshair.","Прицел","Кастомный прицел по центру."},
		{"Crosshair size","Crosshair size","Crosshair arm length in pixels.","Размер прицела","Длина рисок прицела в пикселях."},
		{"FPS / pos overlay","FPS overlay","FPS and position readout in the bottom-left corner.","FPS / поз. оверлей","FPS и координаты в левом нижнем углу."},
		{"Aimbot","Aimbot","Silent aim at head or center within FOV. Hold the aim key.","Аимбот","Сайлент-наведение в голову или центр в пределах FOV. Держи клавишу аима."},
		{"Auto shoot","Auto shoot","Fires automatically while a target is locked.","Авто-огонь","Автоматический огонь при захвате цели."},
		{"Silent aim","Silent aim","The server sees aimed angles, your screen stays still.","Сайлент-аим","Сервер видит наведённые углы, твой экран стоит на месте."},
		{"Target commons","Target commons","Aimbot and triggerbot also lock common infected and the witch, not just specials.","Таргет: обычные","Аимбот и триггер берут обычных заражённых и ведьму, а не только особых."},
		{"Target specials","Target specials","Aimbot and triggerbot also lock hunters, smokers, jockeys, spitters, chargers and the tank.","Таргет: особые","Аимбот и триггер берут охотников, курильщиков, жокеев, плевальщиц, громил и танка."},
		{"Aim FOV x10","Aim FOV","Target search radius around the crosshair, in 0.1 degrees.","FOV аима","Радиус поиска цели вокруг прицела, десятые градуса."},
		{"Smoothing","Smoothing","0 snaps instantly. Higher values look more human.","Сглаживание","0 — мгновенно. Выше — человечнее."},
		{"Aimbot key","Aimbot key","Hold to enable the aimbot. Click to rebind, ESC clears.","Клавиша аима","Держи для работы аимбота. Клик — смена, ESC — сброс."},
		{"Trigger bot","Trigger bot","Shoots when the crosshair is on a visible enemy.","Триггербот","Выстрел, когда прицел на видимом враге."},
		{"Trigger key","Trigger key","Hold to enable the triggerbot. Click to rebind.","Клавиша триггера","Держи для работы триггера. Клик — смена."},
		{"Auto pistol","Auto pistol","Re-clicks semi-auto pistols for hold-to-fire.","Авто-пистолет","Дожимает полуавто-пистолеты для огня с зажатой кнопкой."},
		{"Auto shove","Auto shove","Auto-shoves tongue and pounce attackers off teammates.","Авто-толчок","Авто-толчок: сбрасывает язык и прыгунов с союзников."},
		{"No spread","No spread","Compensates weapon spread and punch in view angles while firing.","Без разброса","Компенсирует разброс и отдачу в углах обзора во время огня."},
		{"Killfeed","Killfeed","Death notices panel in the top-right corner.","Киллфид","Панель смертей в правом верхнем углу."},
		{"Radar","Radar","Top-down 2D radar, up is where you look.","Радар","2D-радар сверху, вверху — куда смотришь."},
		{"Spectators","Spectators","Who is spectating you right now.","Наблюдатели","Кто сейчас смотрит за тобой."},
		{"Alerts","Alerts","Big threat banners: tank spawned, witch nearby.","Алерты","Крупные баннеры угроз: танк заспавнился, ведьма рядом."},
		{"Tank alert","Tank alert","Red TANK banner with distance.","Алерт танка","Красный баннер ТАНК с дистанцией."},
		{"Witch alert","Witch alert","Purple WITCH banner with distance.","Алерт ведьмы","Фиолетовый баннер ВЕДЬМА с дистанцией."},
		{"Save config","Save config","Writes all settings to ZenWare.cfg.","Сохранить конфиг","Пишет все настройки в ZenWare.cfg."},
		{"Load config","Load config","Reads settings back from ZenWare.cfg.","Загрузить конфиг","Читает настройки обратно из ZenWare.cfg."},
		{"Menu key","Menu key","Opens and closes this menu. Click to rebind.","Клавиша меню","Открывает и закрывает меню. Клик — смена."},
		{"Language: English >","Language","Switches the whole menu between English and Russian.","Язык","Переключает всё меню между английским и русским."},
		{"Language: Russian >","Language","Switches the whole menu between English and Russian.","Язык","Переключает всё меню между английским и русским."},
		{"Menu accent","Menu accent","Main accent color of the whole menu.","Акцент меню","Главный акцентный цвет всего меню."},
		{"ESP enemy","ESP enemy","Box color for enemies and specials.","ESP враги","Цвет боксов врагов и особых."},
		{"ESP ally","ESP ally","Box color for teammates.","ESP союзники","Цвет боксов союзников."},
		{"Crosshair","Crosshair","Color of the custom crosshair.","Прицел","Цвет кастомного прицела."},
	};
	static const HelpEntry_t* FindHelp(const char* szLabel){
		for(size_t i=0;i<sizeof(kHelp)/sizeof(kHelp[0]);i++)
			if(!strcmp(kHelp[i].label,szLabel)) return &kHelp[i];
		return nullptr;
	}
	static const char* HelpTitle(const HelpEntry_t* he){ return (he&&Lang::IsRu()&&he->ruTitle)?he->ruTitle:(he?he->title:""); }
	static const char* HelpText(const HelpEntry_t* he){ return (he&&Lang::IsRu()&&he->ruText)?he->ruText:(he?he->text:""); }
}
static std::map<std::string,float> s_tog;
static std::map<std::string,float> s_hover; // плавный hover-отклик строк
static std::map<std::string,float> s_press; // тактильный press-отклик
static float s_menuDt=0.016f; // обновляется в Render
static float HoverAnim(const char* szKey,bool bTarget){
 float& fl=s_hover[szKey];
 fl=Anim::Approach(fl,bTarget?1.0f:0.0f,s_menuDt,18.0f);
 return fl;
}
static float PressAnim(const char* szKey,bool bClicked){
 float& fl=s_press[szKey];
 if(bClicked) fl=1.0f;
 fl=Anim::Approach(fl,0.0f,s_menuDt,14.0f);
 return fl;
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
 // F7 вслепую переключает RU/EN (друг с битым шрифтом не прочитает меню).
 // Работает и при закрытом меню: Render крутится каждый кадр.
 static bool s_bLangInit=false;
 if(!s_bLangInit){ s_bLangInit=true; Vars::Menu::bRussian=(PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_RUSSIAN); }
 // анимация появления (fade-in) - не блокирует логику открытия
 static float s_alpha = 0.0f;
 bool bOpen = HandleOpenState();
 float dt = I::GlobalVars ? I::GlobalVars->frametime : 0.016f;
 if(dt <= 0 || dt > 0.1f) dt = 0.016f;
 m_flDt = dt; s_menuDt = dt;
 m_flAnim = s_alpha;
 s_alpha = Anim::Approach(s_alpha, bOpen ? 1.0f : 0.0f, dt, 9.0f);
 if(s_alpha < 0.01f){
  static bool s_p=false; const bool bF11=(GetAsyncKeyState(VK_F11)&0x8000)!=0; if(bF11&&!s_p) G::ModuleEntry.RequestUnload(); s_p=bF11;
  m_szHelpId=nullptr; m_szHelpTitle=nullptr; m_szHelpText=nullptr;
  if(!Vars::Menu::bOpen && G::Draw.m_nScreenW > 0){
   const float whue=fmodf((float)GetTickCount64()/38.0f,360.0f);
   G::Draw.String(EFonts::MENU_TAHOMA,G::Draw.m_nScreenW-178,G::Draw.m_nScreenH-26,HsvToColor(whue,0.7f,1.0f),TXT_DEFAULT,"ZenWare.cc | %d fps",(int)(1.0f/m_flDt));
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
 m_rc.nX=m_nPosX; m_rc.nY=m_nPosY;
 //красивое открытие/закрытие: fade + scale от центра панели + лёгкий подъем при открытии
 {
  const float flA=Anim::EaseOutCubic(s_alpha);
  const float flScale=0.85f+0.15f*flA;       //0.85 при закрытии -> 1.0 при открытии
  const int nShrinkW=(int)(PANEL_W*(1.0f-flScale)*0.5f);
  const int nShrinkH=(int)(PANEL_H*(1.0f-flScale)*0.5f);
  m_rc.nX+=nShrinkW;
  m_rc.nY+=nShrinkH-(int)((1.0f-flA)*16);
  m_rc.nW=PANEL_W-nShrinkW*2;
  m_rc.nH=PANEL_H-nShrinkH*2;
 }
 if(s_alpha > 0.02f) G::Draw.Rect(0,0,G::Draw.m_nScreenW,G::Draw.m_nScreenH,Color(0,0,0,(int)(70*s_alpha)));
 CLR_ACCENT = Vars::Menu::clrAccent;
 CLR_ACCENT_SOFT = Color(Vars::Menu::clrAccent.r(),Vars::Menu::clrAccent.g(),Vars::Menu::clrAccent.b(),45);
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
     Checkbox(mouse,"ESP special infected",&Vars::ESP::bSpecialBoxes);
     Checkbox(mouse,"ESP witch",&Vars::ESP::bBossBoxes);
     Checkbox(mouse,"Snaplines",&Vars::ESP::bSnaplines);
     Checkbox(mouse,"Filled boxes",&Vars::ESP::bFilled);
     Checkbox(mouse,"HP text near bar",&Vars::ESP::bHealthText);
     Checkbox(mouse,"Weapon text",&Vars::ESP::bWeaponText);
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
     Checkbox(mouse,"Auto duck",&Vars::BunnyHop::bAutoDuck);
    break;
   }
   case 2:{
    SliderInt(mouse,"FOV x100 (view+model)",&Vars::Visuals::nViewFOVSlider,50,300);
    Checkbox(mouse,"No fog",&Vars::Visuals::bNoFog);
    Checkbox(mouse,"Third person",&Vars::Visuals::bThirdPerson);
    if(Vars::Visuals::bThirdPerson) SliderInt(mouse,"3rd person distance",&Vars::Visuals::nThirdPersonDist,30,200);
    Checkbox(mouse,"Crosshair",&Vars::Visuals::bCrosshair);
    SliderInt(mouse,"Crosshair size",&Vars::Visuals::nCrosshairSize,2,30);
    Checkbox(mouse,"FPS / pos overlay",&Vars::Visuals::bOverlay);
    Checkbox(mouse,"Killfeed",&Vars::Killfeed::bEnabled);
    Checkbox(mouse,"Radar",&Vars::Radar::bEnabled);
    Checkbox(mouse,"Spectators",&Vars::Radar::bSpectators);
    Checkbox(mouse,"Alerts",&Vars::Alerts::bEnabled);
    if (Vars::Alerts::bEnabled)
    {
     Checkbox(mouse,"Tank alert",&Vars::Alerts::bTank);
     Checkbox(mouse,"Witch alert",&Vars::Alerts::bWitch);
    }
    break;
   }
   case 3:{
    Checkbox(mouse,"Aimbot",&Vars::Aimbot::bEnabled);
    Checkbox(mouse,"Auto shoot",&Vars::Aimbot::bAutoShoot);
    Checkbox(mouse,"Silent aim",&Vars::Aimbot::bSilent);
    static char szHitbox[32]; sprintf_s(szHitbox,"Hitbox: %s >",Vars::Aimbot::nHitbox?"Center":"Head");
    Button(mouse,szHitbox,[](){ Vars::Aimbot::nHitbox^=1; });
    static char szPrio[32]; sprintf_s(szPrio,"Priority: %s >",Vars::Aimbot::nTargetPriority?"Distance":"FOV");
    Button(mouse,szPrio,[](){ Vars::Aimbot::nTargetPriority^=1; });
    Checkbox(mouse,"Visible only",&Vars::Aimbot::bVisibleOnly);
    Checkbox(mouse,"Skip incapped",&Vars::Aimbot::bIgnoreIncapped);
    Checkbox(mouse,"Target commons",&Vars::Aimbot::bTargetCommons);
    Checkbox(mouse,"Target specials",&Vars::Aimbot::bTargetSpecials);
    SliderInt(mouse,"Aim FOV x10",&Vars::Aimbot::nFOVSlider,5,300);
    SliderInt(mouse,"Smoothing",&Vars::Aimbot::nSmoothSlider,0,60);
    BindRow(mouse,"Aimbot key",&Vars::Aimbot::nKey);
    Checkbox(mouse,"Trigger bot",&Vars::TriggerBot::bEnabled);
    Checkbox(mouse,"Trigger visible only",&Vars::TriggerBot::bVisibleOnly);
    BindRow(mouse,"Trigger key",&Vars::TriggerBot::nKey);
    Checkbox(mouse,"Auto pistol",&Vars::AutoPistol::bEnabled);
    Checkbox(mouse,"Auto shove",&Vars::AutoShove::bEnabled);
    Checkbox(mouse,"No spread",&Vars::NoSpread::bEnabled);
    break;
   }
  default:{
    Button(mouse,"Save config",[](){F::Config.Save();});
    Button(mouse,"Load config",[](){F::Config.Load();
     // слайдеры - источник правды для меню, подтянем их из загруженных float
     Vars::Aimbot::nFOVSlider=U::Math.Clamp((int)(Vars::Aimbot::flFOV*10.0f),5,300);
     Vars::Aimbot::nSmoothSlider=U::Math.Clamp((int)Vars::Aimbot::flSmoothing,0,60);
     Vars::Visuals::nViewFOVSlider=U::Math.Clamp((int)(Vars::Visuals::flViewFOV*100.0f),50,300);});
    BindRow(mouse,"Menu key",&Vars::Menu::nKey);
    static char szLang[32]; sprintf_s(szLang,"Language: %s >",Vars::Menu::bRussian?"Russian":"English");
    Button(mouse,szLang,[](){ Vars::Menu::bRussian=!Vars::Menu::bRussian; });
    SectionLabel("STYLE");
    ColorSwatches(mouse,"Menu accent",&Vars::Menu::clrAccent);
    ColorSwatches(mouse,"ESP enemy",&Vars::Chams::clrEnemy);
    ColorSwatches(mouse,"ESP ally",&Vars::Chams::clrAlly);
    ColorSwatches(mouse,"Crosshair",&Vars::Visuals::clrCrosshair);
   G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+20,m_nItemY+6,CLR_TEXT_OFF,TXT_DEFAULT,Lang::T("F11 = unload cheat"));
   m_nItemY+=26; break;
  }
 }
 const float fhue=fmodf((float)GetTickCount64()/38.0f,360.0f);
 G::Draw.GradientRect(m_rc.nX+1,(m_rc.nY+m_rc.nH)-FOOTER_H-2,m_rc.nX+m_rc.nW-1,(m_rc.nY+m_rc.nH)-FOOTER_H-1,HsvToColor(fhue,0.85f,1.0f),HsvToColor(fhue+140.0f,0.85f,1.0f),true);
 G::Draw.Rect(m_rc.nX+1,(m_rc.nY+m_rc.nH)-FOOTER_H-1,m_rc.nW-2,FOOTER_H,CLR_FOOTER);
 G::Draw.String(EFonts::MENU_CONSOLAS,m_rc.nX+(m_rc.nW/2),(m_rc.nY+m_rc.nH)-FOOTER_H+4,CLR_TEXT_OFF,TXT_CENTERXY,Lang::T("drag header | WASD free | F11 unload | %d fps"),(int)(1.0f/m_flDt));
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
void CFeatures_Menu::SectionLabel(const char* const szLabel){
 G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+12,m_nItemY+2,CLR_TEXT_OFF,TXT_DEFAULT,Lang::T(szLabel));
 m_nItemY+=20;
}
void CFeatures_Menu::ColorSwatches(const MouseState_t& mouse,const char* const szLabel,Color* pValue){
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=24;
 static Color kSw[]={ {0,255,171,255},{255,84,84,255},{255,170,0,255},{255,235,0,255},{120,255,120,255},{0,200,255,255},{90,140,255,255},{190,90,255,255},{255,90,200,255},{235,245,240,255} };
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 float flHov=HoverAnim(szLabel,bHover);
 if(flHov>0.01f){
  G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,Color(255,255,255,(int)(8*flHov)));
  G::Draw.Rect(nRowX,m_nItemY,2,nRowH,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(255*flHov)));
 }
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+12+(int)(2*flHov),m_nItemY+6,CLR_TEXT_OFF,TXT_DEFAULT,Lang::T(szLabel));
 int cr,cg,cb,ca; pValue->GetColor(cr,cg,cb,ca);
 int nX=nRowX+nRowW-10-10*20;
 for(int i=0;i<10;i++){
  bool bHov=Hovered(mouse.pt,nX+i*20,m_nItemY+5,16,14);
  bool bSel=(cr==kSw[i].r()&&cg==kSw[i].g()&&cb==kSw[i].b());
  int nLift=bHov?1:0; // свотч приподнимается под курсором
  G::Draw.Rect(nX+i*20,m_nItemY+5-nLift,16,14,kSw[i]);
  G::Draw.Rect(nX+i*20,m_nItemY+5-nLift,16,14,Color(0,0,0,60)); // мягкая рамка-тень
  G::Draw.Rect(nX+i*20,m_nItemY+6-nLift,16,13,kSw[i]);
  if(bSel) G::Draw.OutlinedRect(nX+i*20,m_nItemY+5-nLift,16,14,Color(255,255,255,255));
  else if(bHov) G::Draw.OutlinedRect(nX+i*20,m_nItemY+5-nLift,16,14,CLR_ACCENT);
  if(bHov&&mouse.bClicked) *pValue=kSw[i];
 }
 m_nItemY+=nRowH;
}
void CFeatures_Menu::DrawPanel(){
 //виньетка: тонкие затемнения сверху/снизу панели для глубины
 G::Draw.Rect(m_rc.nX,m_rc.nY,m_rc.nW,4,Color(255,255,255,10));
 G::Draw.Rect(m_rc.nX,(m_rc.nY+m_rc.nH)-4,m_rc.nW,4,Color(0,0,0,40));
 G::Draw.Rect(m_rc.nX,m_rc.nY,m_rc.nW,m_rc.nH,CLR_BG);
 //тонкая внутренняя рамка-акцент по периметру (премиальная глубина)
 G::Draw.OutlinedRect(m_rc.nX+2,m_rc.nY+2,m_rc.nW-4,m_rc.nH-4,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),28));
 G::Draw.Rect(m_rc.nX,m_rc.nY,2,m_rc.nH,CLR_ACCENT_SOFT);
 G::Draw.Rect(m_rc.nX+2,m_rc.nY,m_rc.nW-2,40,CLR_HEADER);
	DrawRgbLogo(m_rc.nX+16,m_rc.nY+4);
 {
  int hlw = (int)((m_rc.nW - 2) * m_flAnim);
  int hlx = m_rc.nX + 1 + ((m_rc.nW - 2) - hlw) / 2;
  const float hhue=fmodf((float)GetTickCount64()/38.0f,360.0f);
  if (hlw > 0) G::Draw.GradientRect(hlx,m_rc.nY+40,hlx+hlw,m_rc.nY+43,HsvToColor(hhue,0.85f,1.0f),HsvToColor(hhue+40.0f,0.85f,1.0f),false);
 }
 {
  float shx = fmodf((float)GetTickCount64() / 12.0f, (float)(m_rc.nW + 120)) - 60;
  int sbx0 = m_rc.nX + 2 + (int)shx, sbx1 = sbx0 + 60;
  if (sbx0 < m_rc.nX + 2) sbx0 = m_rc.nX + 2;
  if (sbx1 > m_rc.nX + m_rc.nW - 2) sbx1 = m_rc.nX + m_rc.nW - 2;
  if (sbx1 > sbx0) G::Draw.Rect(sbx0, m_rc.nY, sbx1 - sbx0, 40, Color(255,255,255,10));
 }
}
void CFeatures_Menu::Tabs(const MouseState_t& mouse,int& nTab){
 const char* szTabs[]={"Visuals","Move","View","Combat","Misc"};
 const int nTabW=(m_rc.nW-20)/5;
 static float s_pill=0.0f;
 s_pill=Anim::Approach(s_pill,(float)m_nTab,m_flDt,10.0f);
 const int nTabY=m_rc.nY+52; constexpr int nTabH=22;
 const int nPillX=m_rc.nX+10+(int)(s_pill*(float)nTabW);
 G::Draw.Rect(nPillX-2,nTabY-2,nTabW-2,nTabH+4,CLR_ACCENT_SOFT);
 G::Draw.Rect(nPillX,nTabY,nTabW-6,nTabH,CLR_ACCENT);
 G::Draw.Rect(nPillX,nTabY+nTabH-2,nTabW-6,2,CLR_ACCENT_SOFT);
 for(int n=0;n<5;n++){
  int nX=m_rc.nX+10+(n*nTabW); int nY=m_rc.nY+52; constexpr int nH=22;
  bool bActive=(m_nTab==n); bool bHover=Hovered(mouse.pt,nX,nY,nTabW-6,nH);
  Color clrText=bActive?Color(240,255,248,255):(bHover?CLR_TEXT_ON:CLR_TEXT_OFF);
  if(!bActive&&bHover){ G::Draw.Rect(nX,nY,nTabW-6,nH,CLR_ROW_HOVER); G::Draw.Rect(nX,nY+nH-2,nTabW-6,2,CLR_ACCENT_SOFT); }
  G::Draw.String(EFonts::MENU_TAHOMA,nX+((nTabW-6)/2),nY+4,clrText,TXT_CENTERXY,Lang::T(szTabs[n]));
  if(bHover&&mouse.bClicked) m_nTab=n;
 } nTab=m_nTab;
}
static Color LerpC(const Color& a,const Color& b,float t){
 int ar,ag,ab,aa,br,bg2,bb2,ba;
 a.GetColor(ar,ag,ab,aa); b.GetColor(br,bg2,bb2,ba);
 if(t<0) t=0; if(t>1) t=1;
 return Color(ar+(int)((br-ar)*t),ag+(int)((bg2-ag)*t),ab+(int)((bb2-ab)*t),255);
}
void CFeatures_Menu::Checkbox(const MouseState_t& mouse,const char* szLabel,bool* pValue){
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=24;
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 float flHov=HoverAnim(szLabel,bHover);
 float flPress=PressAnim(szLabel,mouse.bClicked&&bHover);
 if(flHov>0.01f){
  G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,Color(255,255,255,(int)(8*flHov)));
  G::Draw.Rect(nRowX,m_nItemY,2,nRowH,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(255*flHov)));
 }
 // toggle track 32x16
 constexpr int nTogW=30, nTogH=14;
 int nTogX=nRowX+nRowW-nTogW-10;
 int nTogY=m_nItemY+(nRowH-nTogH)/2;
 float &flTog=s_tog[szLabel];
 flTog=Anim::Approach(flTog,*pValue?1.0f:0.0f,s_menuDt,12.0f);
 Color trackClr=LerpC(CLR_OUTLINE,CLR_ACCENT,flTog);
 G::Draw.Rect(nTogX,nTogY,nTogW,nTogH,trackClr);
 G::Draw.OutlinedRect(nTogX,nTogY,nTogW,nTogH,LerpC(CLR_OUTLINE_SOFT,CLR_ACCENT,flTog));
 // knob (сжимается при клике)
 int nKnobX=nTogX+7+(int)((nTogW-14)*flTog);
 int nKnobR=(int)(5.0f-1.5f*flPress);
 if(flTog>0.3f) G::Draw.Circle(nKnobX,nTogY+nTogH/2,nKnobR+3,14,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(35*flTog)));
 G::Draw.Circle(nKnobX,nTogY+nTogH/2,nKnobR,16,Color(245,255,250,255));
 const char* szShow=Lang::T(szLabel);
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+12+(int)(2*flHov),m_nItemY+6+(int)(1*flPress),*pValue?CLR_TEXT_ON:CLR_TEXT_OFF,TXT_DEFAULT,szShow);
 bool bHelp=false;
 if(const HelpEntry_t* he=FindHelp(szLabel)){
  const int nQX=nRowX+12+G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,szShow)+7;
  bHelp=HelpMark(mouse,he->label,HelpTitle(he),HelpText(he),nQX,m_nItemY+5);
 }
 if(bHover&&mouse.bClicked&&!bHelp) *pValue=!(*pValue);
 m_nItemY+=nRowH;
}
void CFeatures_Menu::Button(const MouseState_t& mouse,const char* szLabel,void(*pfnAction)()){
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=26;
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 bool bClick=(bHover&&mouse.bClicked);
 float flHov=HoverAnim(szLabel,bHover);
 float flPress=PressAnim(szLabel,bClick);
 if(flHov>0.01f){
  G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,Color(255,255,255,(int)(8*flHov)));
  G::Draw.Rect(nRowX,m_nItemY,2,nRowH,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(255*flHov)));
 }
 // фон кнопки: hover подсвечивает, клик вспыхивает акцентом
 Color bg=CLR_HEADER;
 if(flHov>0.01f) bg=LerpC(CLR_HEADER,CLR_ACCENT,0.07f*flHov);
 if(flPress>0.01f) bg=LerpC(bg,CLR_ACCENT,0.5f*flPress);
 G::Draw.Rect(nRowX+8,m_nItemY+3+(int)(1*flPress),96,nRowH-7-(int)(1*flPress),bg);
 G::Draw.OutlinedRect(nRowX+8,m_nItemY+3+(int)(1*flPress),96,nRowH-7-(int)(1*flPress),(bHover||flPress>0.01f)?CLR_ACCENT:CLR_OUTLINE);
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+8+48,m_nItemY+7+(int)(1*flPress),bHover?CLR_ACCENT:CLR_TEXT_ON,TXT_CENTERXY,Lang::T(szLabel));
 bool bHelp=false;
 if(const HelpEntry_t* he=FindHelp(szLabel))
  bHelp=HelpIcon(mouse,he->label,HelpTitle(he),HelpText(he),nRowX,nRowW,m_nItemY,nRowH);
 if(bClick&&!bHelp&&pfnAction) pfnAction();
 m_nItemY+=nRowH;
}
void CFeatures_Menu::BindRow(const MouseState_t& mouse,const char* szLabel,int* pValue){
 static int* s_pCapturing=nullptr;
 const int nRowX=m_rc.nX+10, nRowW=m_rc.nW-20; constexpr int nRowH=24;
 bool bHover=Hovered(mouse.pt,nRowX,m_nItemY,nRowW,nRowH);
 float flHov=HoverAnim(szLabel,bHover);
 if(flHov>0.01f){
  G::Draw.Rect(nRowX,m_nItemY,nRowW,nRowH,Color(255,255,255,(int)(8*flHov)));
  G::Draw.Rect(nRowX,m_nItemY,2,nRowH,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(255*flHov)));
 }
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+12+(int)(2*flHov),m_nItemY+6,CLR_TEXT_ON,TXT_DEFAULT,Lang::T(szLabel));
 char szVal[32]={};
 if(s_pCapturing==pValue){
  strcpy_s(szVal,Lang::T("[press key]"));
  for(int vk=0x08;vk<0xFE;vk++){ if(vk==VK_LBUTTON||vk==VK_RBUTTON||vk==VK_MBUTTON) continue; if(GetAsyncKeyState(vk)&0x8000){ if(vk==VK_ESCAPE) *pValue=0; else *pValue=vk; s_pCapturing=nullptr; break; } }
 } else if(!*pValue){ sprintf_s(szVal,"[%s]",Lang::T("off")); }
 else { strcpy_s(szVal,"["); strcat_s(szVal,KeyName(*pValue)); strcat_s(szVal,"]"); }
 Color clrVal=(s_pCapturing==pValue)?CLR_ACCENT:CLR_TEXT_ON;
 if(s_pCapturing==pValue){ int pp=150+(int)(105*sinf((GetTickCount64()%6283)/1000.0f)); clrVal=Color(0,255,171,pp); }
 G::Draw.String(EFonts::MENU_TAHOMA,nRowX+nRowW-90,m_nItemY+6,clrVal,TXT_DEFAULT,szVal);
 bool bHelp=false;
 if(const HelpEntry_t* he=FindHelp(szLabel)){
  const int nQX=nRowX+12+G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,Lang::T(szLabel))+7;
  bHelp=HelpMark(mouse,he->label,HelpTitle(he),HelpText(he),nQX,m_nItemY+5);
 }
 if(bHover&&mouse.bClicked&&s_pCapturing!=pValue&&!bHelp) s_pCapturing=pValue;
 m_nItemY+=nRowH;
}
void CFeatures_Menu::LabelInt(const char* szLabel,const int nValue,int nRightPad){
 G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+20,m_nItemY,CLR_TEXT_ON,TXT_DEFAULT,"%s",Lang::T(szLabel));
 char szVal[16]={}; sprintf_s(szVal,"%i",nValue);
 G::Draw.String(EFonts::MENU_TAHOMA,m_rc.nX+m_rc.nW-44-nRightPad,m_nItemY,CLR_ACCENT,TXT_DEFAULT,szVal);
 m_nItemY+=G::Draw.GetFontHeight(EFonts::MENU_TAHOMA)+5;
}
void CFeatures_Menu::SliderInt(const MouseState_t& mouse,const char* szLabel,int* pValue,const int nMin,const int nMax){
 const int nLblY=m_nItemY;
 LabelInt(szLabel,*pValue);
 if(const HelpEntry_t* he=FindHelp(szLabel)){
  const int nQX=m_rc.nX+20+G::Draw.GetTextWidth(EFonts::MENU_TAHOMA,Lang::T(szLabel))+7;
  HelpMark(mouse,he->label,HelpTitle(he),HelpText(he),nQX,nLblY-1);
 }
 int nX=m_rc.nX+20, nW=m_rc.nW-40; constexpr int nTrackH=4, nKnobR=5;
 bool bOnTrack=Hovered(mouse.pt,nX-8,m_nItemY-8,nW+16,nTrackH+16);
 bool bDrag=(mouse.bDown&&bOnTrack);
 float flHov=HoverAnim(szLabel,bOnTrack);
 float flDrag=PressAnim(szLabel,bDrag); // растёт при drag, плавно спадает
 float flFrac=((*pValue-nMin)/static_cast<float>(nMax-nMin)); int nFillW=int(nW*flFrac);
 G::Draw.Rect(nX,m_nItemY,nW,nTrackH,CLR_HEADER);
 G::Draw.Rect(nX,m_nItemY,nFillW,nTrackH,CLR_ACCENT);
 if(nFillW>2) G::Draw.Rect(nX+nFillW-6,m_nItemY,6,nTrackH,Color(200,255,240,255));
 // glow и knob растут при drag
 float flKnobR=nKnobR+1.5f*flDrag;
 G::Draw.Circle(nX+nFillW,m_nItemY+nTrackH/2,(int)(nKnobR+4+4*flDrag),14,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(35+50*flDrag)));
 if(flHov>0.01f) G::Draw.Circle(nX+nFillW,m_nItemY+nTrackH/2,(int)(nKnobR+3),14,Color(CLR_ACCENT.r(),CLR_ACCENT.g(),CLR_ACCENT.b(),(int)(30*flHov)));
 G::Draw.Circle(nX+nFillW,m_nItemY+nTrackH/2,(int)flKnobR,14,CLR_ACCENT);
 if(bDrag){
  float flNew=U::Math.Clamp((mouse.pt.x-nX)/static_cast<float>(nW),0.0f,1.0f);
  *pValue=nMin+int(flNew*(nMax-nMin));
 }
 m_nItemY+=20;
}
