// ZenWare Loader v3.1 - clean dark UI, RGB glowing logo, Steam game launch.
#include <windows.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <windowsx.h>
#include <gdiplus.h>
#include "ManualMapper.h"
#include "Utils.h"
#include "resource.h"
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

namespace {
constexpr int WINDOW_W = 620;
constexpr int WINDOW_H = 300;
constexpr int IDC_PATH = 1001;
constexpr int IDC_BROWSE = 1002;
constexpr int IDC_INJECT = 1003;
constexpr int IDC_STATUS = 1005;
constexpr int IDC_LAUNCH = 1008;
constexpr int IDC_LABEL = 1006;

struct Theme_t {
 COLORREF bg, ctl, text, dim, accent, accent2, red, red2, border;
 bool dark;
};

bool IsSystemDark(){
 DWORD v=0, s=sizeof(v);
 if(RegGetValueA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize","AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&v,&s)==ERROR_SUCCESS) return v==0;
 return true;
}
Theme_t MakeTheme(bool d){
 Theme_t t{}; t.dark=d;
 if(d){ t.bg=RGB(12,14,13); t.ctl=RGB(22,29,25); t.text=RGB(232,255,245); t.dim=RGB(118,142,132); t.accent=RGB(0,255,171); t.accent2=RGB(0,170,113); t.red=RGB(255,84,84); t.red2=RGB(178,32,32); t.border=RGB(42,60,50); }
 else { t.bg=RGB(242,249,244); t.ctl=RGB(255,255,255); t.text=RGB(20,35,30); t.dim=RGB(100,115,110); t.accent=RGB(0,200,135); t.accent2=RGB(0,150,100); t.red=RGB(215,48,48); t.red2=RGB(165,25,25); t.border=RGB(178,218,198); }
 return t;
}
// HSV (h 0..360, s/v 0..1) -> RGB, для радужного логотипа
static COLORREF Hsv(float h, float s, float v){
 while(h<0) h+=360; while(h>=360) h-=360;
 float c=v*s, x=c*(1-fabsf(fmodf(h/60.0f,2.0f)-1.0f)), m=v-c, r=0,g=0,b=0;
 if(h<60){r=c;g=x;} else if(h<120){r=x;g=c;} else if(h<180){g=c;b=x;}
 else if(h<240){g=x;b=c;} else if(h<300){r=x;b=c;} else {r=c;b=x;}
 return RGB((BYTE)((r+m)*255),(BYTE)((g+m)*255),(BYTE)((b+m)*255));
}
Theme_t g_theme = MakeTheme(true);
static float g_flModeT=0.0f, g_flModeTarget=0.0f; // 0=external мятный, 1=internal красный
HWND g_hMain=nullptr, g_hInject=nullptr, g_hStatus=nullptr, g_hLaunch=nullptr;
static bool g_bExternal=true, g_bModeHov=false;
static RECT g_rcMode={0,0,0,0};
// dt-анимации: экспоненциальное сглаживание вместо фиксированного шага
static float g_flHovLaunch=0.0f, g_flHovInject=0.0f; // подсветка кнопок под курсором
static float g_flPressMode=0.0f; // тактильный отклик пилюли режима
static float g_flWinAlpha=0.0f;  // fade-in главного окна
static bool g_bFading=true;
static ULONGLONG g_ullLastTick=0;
static float Approach(float cur,float target,float dt,float speed){ return cur+(target-cur)*(1.0f-expf(-dt*speed)); }
HBRUSH g_brBg=nullptr,g_brCtl=nullptr,g_brBorder=nullptr;
HFONT g_fUI=nullptr,g_fTitle=nullptr,g_fSmall=nullptr;
bool g_busy=false;
bool g_gameSeen=false;
COLORREF g_dotColor=RGB(120,130,124);
void DestroyGdi(){ for(auto h:{&g_brBg,&g_brCtl,&g_brBorder}){ if(*h)DeleteObject(*h);*h=nullptr;} }

void ApplyDwm(){
 BOOL d=g_theme.dark;
 DwmSetWindowAttribute(g_hMain,20,&d,sizeof(d));
 if(FAILED(DwmSetWindowAttribute(g_hMain,20,&d,sizeof(d)))) DwmSetWindowAttribute(g_hMain,19,&d,sizeof(d));
 INT r=2; DwmSetWindowAttribute(g_hMain,33,&r,sizeof(r));
}
void ApplyCtrlTheme(){
}
void RefreshTheme(){
 g_theme=MakeTheme(IsSystemDark());
 DestroyGdi();
 g_brBg=CreateSolidBrush(g_theme.bg);
 g_brCtl=CreateSolidBrush(g_theme.ctl);
 g_brBorder=CreateSolidBrush(g_theme.border);
 ApplyDwm();
 if(g_hMain) InvalidateRect(g_hMain,nullptr,TRUE);
 ApplyCtrlTheme();
}
void InitFonts(HWND hwnd){
 if(g_fUI)DeleteObject(g_fUI); if(g_fTitle)DeleteObject(g_fTitle); if(g_fSmall)DeleteObject(g_fSmall);
 g_fUI=g_fTitle=g_fSmall=nullptr;
 UINT dpi=GetDpiForWindow(hwnd); if(!dpi) dpi=96;
 int h1=-MulDiv(11,(int)dpi,96);
 int h2=-MulDiv(30,(int)dpi,96);
 int h3=-MulDiv(8,(int)dpi,96);
 g_fUI=CreateFontW(h1,0,0,0,600,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
 g_fTitle=CreateFontW(h2,0,0,0,800,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI Black");
 g_fSmall=CreateFontW(h3,0,0,0,600,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
}
// Радужный неоновый логотип: свечение + переливающиеся буквы
void DrawRgbLogo(HDC dc, int x, int y){
 const wchar_t* txt=L"ZenWare.cc";
 SIZE sz={0,0};
 auto oldF=SelectObject(dc,g_fTitle);
 SetBkMode(dc,TRANSPARENT);
 GetTextExtentPoint32W(dc,txt,(int)wcslen(txt),&sz);
 const float hue=fmodf(GetTickCount64()/38.0f,360.0f);
 // свечение: 8 копий вокруг со сдвигом, тёмный радужный цвет
 for(int dx=-2;dx<=2;dx+=2) for(int dy=-2;dy<=2;dy+=2){
  if(!dx&&!dy) continue;
   SetTextColor(dc,Hsv(hue,0.9f,0.35f));
  TextOutW(dc,x+dx,y+dy,txt,(int)wcslen(txt));
 }
 // буквы радугой
 int cx=x;
 for(const wchar_t* p=txt;*p;++p){
  int idx=(int)(p-txt);
   SetTextColor(dc,Hsv(hue+idx*5.0f,0.85f,1.0f));
  wchar_t ch[2]={*p,0};
  SIZE cs={0,0}; GetTextExtentPoint32W(dc,ch,1,&cs);
  TextOutW(dc,cx,y,ch,1);
  cx+=cs.cx;
 }
 SelectObject(dc,oldF);
}
struct Ctx{ DWORD pid; std::wstring path; bool manual; };
DWORD WINAPI InjectThread(LPVOID p){
 auto c=(Ctx*)p;
 g_busy=true; EnableWindow(g_hInject,FALSE);
 CManualMapper m; CManualMapper::Params_t pr{};
 pr.hwndLog=g_hMain; pr.dwTargetPid=c->pid; pr.wszDllPath=c->path; pr.pfnLog=&LoaderUtil::Log; pr.pfnStatus=&LoaderUtil::Status;
 bool ok = c->manual ? m.Map(pr) : CManualMapper::InjectStandard(pr);
 if(!ok){ LoaderUtil::Log(g_hMain,"[!] Injection FAILED"); LoaderUtil::Status(g_hMain, LoaderUtil::S("Ошибка","Error")); }
 EnableWindow(g_hInject,TRUE); g_busy=false; delete c; return 0;
}
static bool FindDll(wchar_t* out){
 const wchar_t* cands[] = { L"/ZenWare.dll", L"/../../../ZenWare.DLL/bin/Release/ZenWare.dll", L"C:/Users/ilya/Desktop/ZenWare.cc/ZenWare.DLL/bin/Release/ZenWare.dll" };
 wchar_t dir[MAX_PATH]={};
 GetModuleFileNameW(NULL,dir,MAX_PATH);
 wchar_t* s=wcsrchr(dir,92); if(s) *s=0;
 for(auto rel:cands){
  wchar_t t[MAX_PATH]={}, f[MAX_PATH]={};
  wcscpy_s(t,dir); wcscat_s(t,rel);
  GetFullPathNameW(t,MAX_PATH,f,nullptr);
  if(GetFileAttributesW(f)!=INVALID_FILE_ATTRIBUTES){ wcscpy_s(out,MAX_PATH,f); return true; }
 }
 return false;
}
void ToggleMode(){
 g_bExternal=!g_bExternal;
 g_flModeTarget=g_bExternal?0.0f:1.0f;
 SetWindowTextW(g_hInject,LoaderUtil::SW(g_bExternal?L"ЗАПУСК EXTERNAL":L"ИНЖЕКТ",g_bExternal?L"LAUNCH EXTERNAL":L"INJECT"));
 LoaderUtil::Status(g_hMain,LoaderUtil::S(g_bExternal?"Режим: External (отдельный процесс)":"Режим: Internal (инжект DLL)",g_bExternal?"Mode: External (own process)":"Mode: Internal (DLL inject)"));
 RECT hdr={0,0,WINDOW_W,76}; InvalidateRect(g_hMain,&hdr,FALSE);
}
void LaunchExternal(){
 wchar_t dir[MAX_PATH]={}; GetModuleFileNameW(NULL,dir,MAX_PATH);
 wchar_t* s=wcsrchr(dir,L'\\'); if(s) *s=0;
 const wchar_t* cands[]={L"\\ZenWare.External.exe",L"\\..\\..\\ZenWare.External\\bin\\Release\\ZenWare.External.exe",L"C:\\Users\\ilya\\Desktop\\ZenWare.cc\\ZenWare.External\\bin\\Release\\ZenWare.External.exe"};
 wchar_t goods[MAX_PATH]={};
 wchar_t tried[3][MAX_PATH]={};
 for(int i=0;i<3;i++){
  wchar_t t[MAX_PATH]={}, f[MAX_PATH]={};
  const wchar_t* rel=cands[i];
  if((rel[0]==L'\\'&&rel[1]==L'\\')||rel[1]==L':') wcscpy_s(t,rel);
  else { wcscpy_s(t,dir); wcscat_s(t,rel); }
  GetFullPathNameW(t,MAX_PATH,f,nullptr);
  wcscpy_s(tried[i],f);
  if(GetFileAttributesW(f)!=INVALID_FILE_ATTRIBUTES){ wcscpy_s(goods,MAX_PATH,f); break; }
 }
 if(!goods[0]){
  LoaderUtil::Status(g_hMain,LoaderUtil::S("External не найден — собери проект","External not found — build it"));
  LoaderUtil::Log(g_hMain,"[!] External exe not found, tried:");
  for(int i=0;i<3;i++){ char nb[MAX_PATH*2]={}; WideCharToMultiByte(CP_ACP,0,tried[i],-1,nb,sizeof(nb),nullptr,nullptr); LoaderUtil::Log(g_hMain,"[?] %s",nb); }
  return;
 }
 ShellExecuteW(nullptr,L"open",goods,nullptr,nullptr,SW_SHOWNORMAL);
 LoaderUtil::Status(g_hMain,LoaderUtil::S("External запущен","External launched"));
}
void StartInject(){
 wchar_t p[MAX_PATH]={};
 if(!FindDll(p)){ MessageBoxW(g_hMain, LoaderUtil::SW(L"DLL не найдена рядом с лоадером",L"DLL not found next to loader"), L"ZenWare", MB_ICONWARNING); return;}
 if(g_busy) return;
 LoaderUtil::Status(g_hMain, LoaderUtil::S("Поиск процесса","Finding process"));
 DWORD pid=LoaderUtil::FindProcessId(L"left4dead2.exe");
 if(!pid){ LoaderUtil::Status(g_hMain, LoaderUtil::S("Игра не найдена","Game not found")); return;}
 auto c=new Ctx{pid,p,false};
 CreateThread(nullptr,0,InjectThread,c,0,nullptr);
}
void LaunchGame(){
 wchar_t steam[MAX_PATH]={}; DWORD sz=sizeof(steam);
 // 1) полный путь из реестра
 if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamExe",RRF_RT_REG_SZ,nullptr,steam,&sz)!=ERROR_SUCCESS){
  // 2) папка Steam + steam.exe
  wchar_t dir[MAX_PATH]={}; DWORD dz=sizeof(dir);
  if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath",RRF_RT_REG_SZ,nullptr,dir,&dz)==ERROR_SUCCESS){
   wcscpy_s(steam,dir); wcscat_s(steam,L"\\steam.exe");
  }
 }
 if(!steam[0] || GetFileAttributesW(steam)==INVALID_FILE_ATTRIBUTES){
  LoaderUtil::Status(g_hMain, LoaderUtil::S("Steam не найден","Steam not found"));
  return;
 }
 // -applaunch 550 = Left 4 Dead 2, дальше аргументы уходят игре
 ShellExecuteW(nullptr,L"open",steam,L"-applaunch 550 -novid -console",nullptr,SW_SHOWNORMAL);
 LoaderUtil::Status(g_hMain, LoaderUtil::S("Запуск игры через Steam...","Launching via Steam..."));
 g_gameSeen=false;
}
static COLORREF Mix2(COLORREF a, COLORREF b, int t){
 int r=(GetRValue(a)*(255-t)+GetRValue(b)*t)/255;
 int g=(GetGValue(a)*(255-t)+GetGValue(b)*t)/255;
 int bl=(GetBValue(a)*(255-t)+GetBValue(b)*t)/255;
 return RGB(r,g,bl);
}
// Плавный акцент темы: external мятный -> internal красный
static COLORREF LerpC2(COLORREF a,COLORREF b,float t){
 if(t<0)t=0; if(t>1)t=1;
 return RGB((int)(GetRValue(a)+(GetRValue(b)-GetRValue(a))*t),(int)(GetGValue(a)+(GetGValue(b)-GetGValue(a))*t),(int)(GetBValue(a)+(GetBValue(b)-GetBValue(a))*t));
}
static bool g_bParty=false;
static COLORREF Acc(){ if(g_bParty){ float hue=fmodf((float)GetTickCount64()/38.0f,360.0f); return Hsv(hue,0.85f,1.0f); } Theme_t& th=g_theme; return LerpC2(th.accent,th.red,g_flModeT); }
static COLORREF Acc2(){ if(g_bParty){ float hue=fmodf((float)GetTickCount64()/38.0f,360.0f); return Hsv(hue,0.9f,0.6f); } Theme_t& th=g_theme; return LerpC2(th.accent2,th.red2,g_flModeT); }
static void ClassifyStatus(const wchar_t* t){
 if(!t) return;
 auto has=[](const wchar_t* h,const wchar_t* n){ return wcsstr(h,n)!=nullptr; };
 if(has(t,L"Ошибка")||has(t,L"Error")||has(t,L"FAILED")||has(t,L"не найден")||has(t,L"not found")||has(t,L"[!]"))
  g_dotColor=RGB(255,80,80);
 else if(has(t,L"Готово")||has(t,L"Done")||has(t,L"успешно")||has(t,L"success")||has(t,L"[===]"))
  g_dotColor=RGB(0,255,171);
 else if(g_busy) g_dotColor=Acc();
}
LRESULT DrawBtn(LPARAM lp){
  auto d=(LPDRAWITEMSTRUCT)lp; if(!d) return TRUE;
   bool en=IsWindowEnabled(d->hwndItem); bool pri=(d->CtlID==IDC_INJECT);
   bool pr=(d->itemState & ODS_SELECTED)!=0;
   RECT cr; GetClientRect(d->hwndItem,&cr);
 // закрасить всё поле кнопки цветом диалога и обрезать рисование скруглением,
 // иначе по краям остаются неокрашенные белые пиксели
 FillRect(d->hDC,&cr,g_brBg);
 HRGN rgClip=CreateRoundRectRgn(cr.left,cr.top,cr.right+1,cr.bottom+1,12,12);
 SelectClipRgn(d->hDC,rgClip);
  COLORREF fill=g_theme.ctl, txt=g_theme.text, br=Mix2(g_theme.border,Acc(),110);
  float hovF= (d->CtlID==IDC_INJECT)?g_flHovInject : (d->CtlID==IDC_LAUNCH)?g_flHovLaunch : 0.0f;
  if(pri&&en){
   fill=Mix2(Acc(),RGB(255,255,255),(int)(hovF*50)); txt=g_theme.dark?RGB(4,12,8):RGB(255,255,255); br=Acc();
  // вертикальный градиент поверх заливки
  TRIVERTEX vv[2]={{cr.left,cr.top,(COLOR16)(GetRValue(fill)<<8),(COLOR16)(GetGValue(fill)<<8),(COLOR16)(GetBValue(fill)<<8),0},
   {cr.right,cr.bottom,(COLOR16)(GetRValue(Acc2())<<8),(COLOR16)(GetGValue(Acc2())<<8),(COLOR16)(GetBValue(Acc2())<<8),0}};
  GRADIENT_RECT gr={0,1}; GdiGradientFill(d->hDC,vv,2,&gr,1,GRADIENT_FILL_RECT_V);
 } else {
    if(!en){ fill=g_theme.bg; txt=g_theme.dim; }
    else if(pr){ fill=Mix2(g_theme.ctl,Acc(),60); }
    else { fill=Mix2(g_theme.ctl,Acc(),(int)(hovF*36)); br=Mix2(g_theme.border,Acc(),110+(int)(hovF*145)); }
   // кнопка запуска игры: радужная обводка в ритме логотипа
   bool launch=(d->CtlID==IDC_LAUNCH);
   COLORREF rbDim=0;
   if(launch&&en){
    float hue=fmodf(GetTickCount64()/38.0f,360.0f);
    br=Hsv(hue,0.85f,1.0f); rbDim=Hsv(hue,0.9f,0.35f);
   }
   HBRUSH b=CreateSolidBrush(fill); HPEN pen=CreatePen(PS_SOLID,1,br);
   auto o1=SelectObject(d->hDC,b); auto o2=SelectObject(d->hDC,pen);
   RECT r=cr; InflateRect(&r,-1,-1); RoundRect(d->hDC,r.left,r.top,r.right,r.bottom,10,10);
   if(launch&&en){
    // внешнее неоновое свечение тем же оттенком, что у логотипа
    HPEN gp=CreatePen(PS_SOLID,1,rbDim); auto og=SelectObject(d->hDC,gp);
    HGDIOBJ ng=SelectObject(d->hDC,GetStockObject(NULL_BRUSH));
    RoundRect(d->hDC,cr.left,cr.top,cr.right,cr.bottom,11,11);
    SelectObject(d->hDC,ng); SelectObject(d->hDC,og); DeleteObject(gp);
   }
   SelectObject(d->hDC,o1); SelectObject(d->hDC,o2); DeleteObject(b); DeleteObject(pen);
 }
 if(pri&&en){
  HPEN bp=CreatePen(PS_SOLID,1,Acc()); auto ob=SelectObject(d->hDC,bp);
  HGDIOBJ nb=SelectObject(d->hDC,GetStockObject(NULL_BRUSH));
  RECT r=cr; InflateRect(&r,-1,-1); RoundRect(d->hDC,r.left,r.top,r.right,r.bottom,10,10);
  SelectObject(d->hDC,nb); SelectObject(d->hDC,ob); DeleteObject(bp);
 }
  { // стеклянный блик сверху кнопки
   HBRUSH hb=CreateSolidBrush(Mix2(fill,RGB(255,255,255),26));
   RECT hr={cr.left+3,cr.top+1,cr.right-3,cr.top+3}; FillRect(d->hDC,&hr,hb); DeleteObject(hb);
  }
  SetBkMode(d->hDC,TRANSPARENT); SetTextColor(d->hDC,txt); auto o3=SelectObject(d->hDC,g_fUI);
 wchar_t t[64]={}; GetWindowTextW(d->hwndItem,t,64);
 RECT r2=cr; DrawTextW(d->hDC,t,-1,&r2,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 SelectObject(d->hDC,o3);
 SelectClipRgn(d->hDC,nullptr); DeleteObject(rgClip);
 return TRUE;
}
LRESULT ColorChild(UINT msg,WPARAM wp,LPARAM lp){
 HDC dc=(HDC)wp; HWND ctl=(HWND)lp; int id=ctl?GetDlgCtrlID(ctl):0;
 SetTextColor(dc, id==IDC_STATUS?Acc():g_theme.dim); SetBkColor(dc,g_theme.bg); return (LRESULT)g_brBg;
}
// ---------- кодовый интро-сплэш: безрамочное окно ~2.8с, клик пропускает ----------
static constexpr int SPL_W=560, SPL_H=300;
static constexpr ULONGLONG SPLASH_MS=2800;
static HWND g_hSplash=nullptr;
static ULONGLONG g_splashT0=0;
static ULONGLONG g_splashQ[6]={};
struct SplashP_t{ float x0,y0,x1,y1,dl; int sz; };
static SplashP_t g_parts[130];
struct SplashD_t{ float x,y,ph,sp; };
static SplashD_t g_dust[45];
static Gdiplus::Image* g_splashImg=nullptr;
static Gdiplus::Bitmap* g_splashGlow=nullptr;
static Gdiplus::Bitmap* g_logoBase=nullptr;
static ULONGLONG g_nQ2=0;
static unsigned SplashRnd(unsigned& s){ s^=s<<13; s^=s>>17; s^=s<<5; return s; }
static float SplashEase(float t){ if(t<0) t=0; if(t>1) t=1; return t*t*(3.0f-2.0f*t); }
LRESULT CALLBACK SplashProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:{
  g_hSplash=h; g_splashT0=GetTickCount64();
  unsigned s=(unsigned)GetTickCount64()|1u;
  for(int i=0;i<130;i++){
   int e=SplashRnd(s)%4; float ex,ey;
   if(e==0){ ex=(float)(SplashRnd(s)%SPL_W); ey=-12; }
   else if(e==1){ ex=(float)(SplashRnd(s)%SPL_W); ey=SPL_H+12; }
   else if(e==2){ ex=-12; ey=(float)(SplashRnd(s)%SPL_H); }
   else { ex=SPL_W+12; ey=(float)(SplashRnd(s)%SPL_H); }
   g_parts[i]={ex,ey,(float)(SPL_W/2+(int)(SplashRnd(s)%170)-85),(float)(89+(int)(SplashRnd(s)%90)-45),(SplashRnd(s)%600)/1000.0f,2+(int)(SplashRnd(s)%2)};
  }
  for(int i=0;i<45;i++){
   g_dust[i]={(float)(SplashRnd(s)%SPL_W),(float)(SplashRnd(s)%SPL_H),(SplashRnd(s)%628)/100.0f,0.4f+(SplashRnd(s)%100)/140.0f};
  }
  SetLayeredWindowAttributes(h,0,0,LWA_ALPHA);
  break;
 }
 case WM_PAINT:{
  PAINTSTRUCT ps; HDC hdc=BeginPaint(h,&ps);
  HDC mem=CreateCompatibleDC(hdc);
  HBITMAP bmp=CreateCompatibleBitmap(hdc,SPL_W,SPL_H);
  HGDIOBJ obm=SelectObject(mem,bmp);
  HDC dc=mem;
  { // fade поверх кадра, альфу дёргаем только при изменении
   ULONGLONG elA=GetTickCount64()-g_splashT0; BYTE a=255;
   if(elA<300) a=(BYTE)(elA*255/300);
   else if(elA>SPLASH_MS-400) a=(elA>=SPLASH_MS)?0:(BYTE)((SPLASH_MS-elA)*255/400);
   static BYTE lastA=0; if(a!=lastA){ SetLayeredWindowAttributes(h,0,a,LWA_ALPHA); lastA=a; }
  }
  const COLORREF bg=RGB(8,10,9);
  ULONGLONG q0=GetTickCount64(); ULONGLONG q2mark=0;
  HBRUSH bb=CreateSolidBrush(bg); RECT rc={0,0,SPL_W,SPL_H}; FillRect(dc,&rc,bb); DeleteObject(bb);
  const float el=(GetTickCount64()-g_splashT0)/1000.0f;
  const float hue=fmodf((float)GetTickCount64()/38.0f,360.0f);
  // сетка
  {
   int gk=(int)(SplashEase(el/1.2f)*26);
   if(gk>0){
    HPEN gp2=CreatePen(PS_SOLID,1,Mix2(bg,RGB(34,48,40),gk)); auto og2=SelectObject(dc,gp2);
    for(int gx=20;gx<SPL_W;gx+=40){ MoveToEx(dc,gx,0,nullptr); LineTo(dc,gx,SPL_H); }
    for(int gy=20;gy<SPL_H;gy+=40){ MoveToEx(dc,0,gy,nullptr); LineTo(dc,SPL_W,gy); }
    SelectObject(dc,og2); DeleteObject(gp2);
   }
  }
  // пыль
  for(int i=0;i<45;i++){
   const SplashD_t& d=g_dust[i];
   float dyy=d.y-el*9.0f*d.sp; dyy=dyy-(int)(dyy/SPL_H)*SPL_H; if(dyy<0)dyy+=SPL_H;
   float dxx=d.x+sinf(el*0.7f+d.ph)*8.0f;
   int da=18+(int)(16*sinf(el*d.sp*2.0f+d.ph*3.0f));
   if(da<4) continue;
   HBRUSH db=CreateSolidBrush(Mix2(bg,g_theme.dim,da));
   RECT dr={(int)dxx,(int)dyy,(int)dxx+2,(int)dyy+2}; FillRect(dc,&dr,db); DeleteObject(db);
  }
  // частицы со шлейфами
  for(int i=0;i<130;i++){
   const SplashP_t& p=g_parts[i];
   float t=SplashEase((el-p.dl)/1.1f);
   if(t<=0||t>=1) continue;
   for(int s2=0;s2<3;s2++){
    float tt=t-s2*0.035f; if(tt<=0) continue;
    float px=p.x0+(p.x1-p.x0)*tt, py=p.y0+(p.y1-p.y0)*tt;
    int a2=(int)((220*(1.0f-t)+35)/(s2*2+1));
    int sh=p.sz-s2; if(sh<1) sh=1;
    HBRUSH pb=CreateSolidBrush(Mix2(bg,Hsv(hue+i*3.0f,0.85f,1.0f),a2));
    RECT pr={(int)px-sh/2,(int)py-sh/2,(int)px+sh/2+1,(int)py+sh/2+1};
    FillRect(dc,&pr,pb); DeleteObject(pb);
   }
  }
  // ударные кольца
  for(int r2=0;r2<2;r2++){
   float rt0=r2?1.9f:1.2f;
   float rt=(el-rt0)/0.7f;
   if(rt>0&&rt<1){
    int rr=(int)(20+rt*260);
    HPEN rp=CreatePen(PS_SOLID,2,Mix2(bg,Acc(),(int)(110*(1.0f-rt)))); auto ogr=SelectObject(dc,rp);
    HGDIOBJ onb=SelectObject(dc,GetStockObject(NULL_BRUSH));
    Ellipse(dc,SPL_W/2-rr,89-rr,SPL_W/2+rr,89+rr);
    SelectObject(dc,onb); SelectObject(dc,ogr); DeleteObject(rp);
   }
  }
   g_splashQ[0]+=GetTickCount64()-q0; ULONGLONG q1=GetTickCount64();
   // эмблема: настоящий логотип со свечением, ESP-уголками и сканлайном
   float la=SplashEase((el-0.15f)/0.6f);
   const int LCX=SPL_W/2, LCY=89, LSZ=150;
   if(la>0){
    if(g_splashImg){
     Gdiplus::Graphics gd(dc);
     gd.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
     gd.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
     int bob=(int)(4.0f*sinf(el*1.8f));
     int gs=LSZ+68+(int)(8*sinf(el*4.0f));
     if(g_splashGlow) gd.DrawImage(g_splashGlow,LCX-gs/2,LCY+bob-gs/2,gs,gs);
     int L=(el<1.0f)?(int)(LSZ*(0.75f+0.25f*SplashEase(el/0.9f))):LSZ;
     if(g_logoBase) gd.DrawImage(g_logoBase,LCX-L/2,LCY+bob-L/2,L,L);
     float bt=SplashEase((el-0.5f)/0.7f);
     if(bt>0){
      int mg=14+(int)(40*(1.0f-bt));
      int bx0=LCX-LSZ/2-mg, bx1=LCX+LSZ/2+mg, by0=LCY-LSZ/2-mg, by1=LCY+LSZ/2+mg;
      int bl=30, bk=(int)(bt*255);
      HPEN hp=CreatePen(PS_SOLID,2,Mix2(bg,Acc(),bk)); auto oh=SelectObject(dc,hp);
      MoveToEx(dc,bx0,by0+bl,nullptr); LineTo(dc,bx0,by0); LineTo(dc,bx0+bl,by0);
      MoveToEx(dc,bx1-bl,by0,nullptr); LineTo(dc,bx1,by0); LineTo(dc,bx1,by0+bl);
      MoveToEx(dc,bx1,by1-bl,nullptr); LineTo(dc,bx1,by1); LineTo(dc,bx1-bl,by1);
      MoveToEx(dc,bx0+bl,by1,nullptr); LineTo(dc,bx0,by1); LineTo(dc,bx0,by1-bl);
      SelectObject(dc,oh); DeleteObject(hp);
     }
     { // орбита из пунктирных дуг вокруг эмблемы
      float oa=el*0.9f;
      int orad=LSZ/2+30;
      HPEN op=CreatePen(PS_SOLID,2,Mix2(bg,Acc(),(int)(la*130))); auto oo=SelectObject(dc,op);
      for(int k2=0;k2<24;k2+=2){
       float a0=oa+k2*0.2618f, a1=a0+0.16f;
       MoveToEx(dc,LCX+(int)(orad*cosf(a0)),LCY+(int)(orad*sinf(a0)),nullptr);
       LineTo(dc,LCX+(int)(orad*cosf(a1)),LCY+(int)(orad*sinf(a1)));
      }
      SelectObject(dc,oo); DeleteObject(op);
     }
     if(el>0.7f&&el<1.9f){
      int sy2=(LCY-LSZ/2)+(int)(LSZ*((el-0.7f)/1.2f));
      HBRUSH sb=CreateSolidBrush(Mix2(bg,Acc(),(int)(la*90))); RECT sr={LCX-LSZ/2,sy2,LCX+LSZ/2,sy2+2}; FillRect(dc,&sr,sb); DeleteObject(sb);
     }
    }
    g_splashQ[1]+=GetTickCount64()-q1; q2mark=GetTickCount64();
    const wchar_t* txt=L"ZenWare.cc";
   auto of=SelectObject(dc,g_fTitle); SetBkMode(dc,TRANSPARENT);
   SIZE sz={0,0}; GetTextExtentPoint32W(dc,txt,(int)wcslen(txt),&sz);
    int x0=(SPL_W-sz.cx)/2, y0=g_splashImg?172:104;
   int k=(int)(la*255);
   for(int dx=-2;dx<=2;dx+=4) for(int dy=-2;dy<=2;dy+=4){
    if(!dx&&!dy) continue;
    SetTextColor(dc,Mix2(bg,Hsv(hue,0.9f,0.35f),k));
    TextOutW(dc,x0+dx,y0+dy,txt,(int)wcslen(txt));
   }
    int cx=x0;
    float swx=(el-1.6f)/0.8f;
    for(const wchar_t* p=txt;*p;++p){
     int idx=(int)(p-txt);
     // посимвольный вылет: каждая буква выезжает снизу с overshoot-пружинкой
     float lc=SplashEase((el-0.75f-idx*0.045f)/0.5f);
     if(lc<=0.0f){ wchar_t c0[2]={*p,0}; SIZE cs0={0,0}; GetTextExtentPoint32W(dc,c0,1,&cs0); cx+=cs0.cx; continue; }
     float ov=lc<1.0f?(1.0f-lc)*(1.0f-lc)*-26.0f:0.0f; // подпрыгивание в конце
     int dy=(int)((1.0f-lc)*34+ov);
     COLORREF lcol=Mix2(bg,Hsv(hue+idx*5.0f,0.85f,1.0f),k);
     if(swx>0&&swx<1.4f){
      float dd=idx-(swx*12.0f-1.0f);
      float bb2=expf(-dd*dd/1.5f);
      if(bb2>0.03f) lcol=Mix2(lcol,RGB(255,255,255),(int)(bb2*150));
     }
     SetTextColor(dc,lcol);
     wchar_t ch[2]={*p,0}; SIZE cs={0,0}; GetTextExtentPoint32W(dc,ch,1,&cs);
     TextOutW(dc,cx,y0+dy,ch,1); cx+=cs.cx;
    }
   // раскрывающаяся линия + подпись
   float lw=SplashEase((el-0.9f)/0.8f);
   if(lw>0){
     int hw2=(int)(200*lw);
     int ly=g_splashImg?214:196;
     HPEN lp=CreatePen(PS_SOLID,2,Mix2(bg,Acc(),(int)(la*255))); auto ol=SelectObject(dc,lp);
     MoveToEx(dc,SPL_W/2-hw2,ly,nullptr); LineTo(dc,SPL_W/2+hw2,ly);
    SelectObject(dc,ol); DeleteObject(lp);
    auto os=SelectObject(dc,g_fSmall);
    SetTextColor(dc,Mix2(bg,g_theme.dim,(int)(la*255)));
     RECT vr={0,ly+8,SPL_W,ly+28}; DrawTextW(dc,L"LOADER v3.2  •  EXTERNAL x86",-1,&vr,DT_CENTER|DT_SINGLELINE);
    SelectObject(dc,os);
   }
   SelectObject(dc,of);
  }
  if(q2mark){ g_splashQ[2]+=GetTickCount64()-q2mark; g_nQ2++; } ULONGLONG q3=GetTickCount64();
  // прогресс-бар
  {
   float f=el/2.6f; if(f>1) f=1;
   RECT tr={80,252,480,256}; HBRUSH tb=CreateSolidBrush(g_theme.ctl); FillRect(dc,&tr,tb); DeleteObject(tb);
   if(f>0){ RECT fl={80,252,80+(int)(400*f),256}; HBRUSH fb=CreateSolidBrush(Acc()); FillRect(dc,&fl,fb); DeleteObject(fb);
    int hx=80+(int)(400*f);
    HBRUSH hb=CreateSolidBrush(Mix2(Acc(),RGB(255,255,255),120)); RECT hr2={hx-12,251,hx,257}; FillRect(dc,&hr2,hb); DeleteObject(hb); }
  }
   { // финальная вспышка при закрытии: короткая белая заливка с затуханием
    ULONGLONG elF=GetTickCount64()-g_splashT0;
    if(elF>SPLASH_MS-320){
     float ft=(float)(elF-(SPLASH_MS-320))/320.0f;
     if(ft>1.0f) ft=1.0f;
     HBRUSH fb=CreateSolidBrush(Mix2(bg,RGB(255,255,255),(int)(90*(1.0f-ft))));
     RECT fr2={0,0,SPL_W,SPL_H}; FillRect(dc,&fr2,fb); DeleteObject(fb);
    }
   }
   auto os2=SelectObject(dc,g_fSmall); SetTextColor(dc,g_theme.dim);
  RECT hr={0,272,SPL_W-16,292}; DrawTextW(dc,LoaderUtil::SW(L"клик — пропустить",L"click to skip"),-1,&hr,DT_RIGHT|DT_SINGLELINE);
  SelectObject(dc,os2);
  g_splashQ[3]+=GetTickCount64()-q3; ULONGLONG q4=GetTickCount64();
  BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,mem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
  g_splashQ[4]+=GetTickCount64()-q4;
  SelectObject(mem,obm); DeleteObject(bmp); DeleteDC(mem);
  EndPaint(h,&ps);
  break;
 }
 case WM_LBUTTONDOWN: case WM_KEYDOWN:
  KillTimer(h,1); DestroyWindow(h); break;
 case WM_DESTROY:
  KillTimer(h,1); g_hSplash=nullptr; break;
 default: return DefWindowProcW(h,m,w,l);
 }
 return 0;
}
static bool FindLogo(wchar_t* out){
 wchar_t dir[MAX_PATH]={};
 GetModuleFileNameW(NULL,dir,MAX_PATH);
 wchar_t* s=wcsrchr(dir,L'\\'); if(s) *s=0;
 wchar_t t[MAX_PATH]={};
 wcscpy_s(t,dir); wcscat_s(t,L"\\zenwareLOGO.png");
 if(GetFileAttributesW(t)!=INVALID_FILE_ATTRIBUTES){ wcscpy_s(out,MAX_PATH,t); return true; }
 if(GetFileAttributesW(L"zenwareLOGO.png")!=INVALID_FILE_ATTRIBUTES){ wcscpy_s(out,MAX_PATH,L"zenwareLOGO.png"); return true; }
 return false;
}
void RunSplash(HINSTANCE hi){
 ULONG_PTR tok=0; Gdiplus::GdiplusStartupInput si; Gdiplus::GdiplusStartup(&tok,&si,nullptr);
 wchar_t lp[MAX_PATH]={};
 if(FindLogo(lp)){
  g_splashImg=Gdiplus::Image::FromFile(lp);
  if(g_splashImg&&g_splashImg->GetLastStatus()!=Gdiplus::Ok){ delete g_splashImg; g_splashImg=nullptr; }
 }
 if(g_splashImg){ // логотип один раз в готовый спрайт, в кадре только быстрый блит
  g_logoBase=new Gdiplus::Bitmap(150,150,PixelFormat32bppPARGB);
  Gdiplus::Graphics gl(g_logoBase);
  gl.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
  gl.DrawImage(g_splashImg,0,0,150,150);
 }
 // свечение предрендерим один раз в спрайт, а не считаем градиент каждый кадр
 g_splashGlow=new Gdiplus::Bitmap(240,240,PixelFormat32bppPARGB);
 {
  Gdiplus::Graphics gg(g_splashGlow);
  Gdiplus::GraphicsPath gp; gp.AddEllipse(0,0,240,240);
  Gdiplus::PathGradientBrush pb(&gp);
  pb.SetCenterPoint(Gdiplus::PointF(120,120));
  pb.SetCenterColor(Gdiplus::Color(255,0,255,171));
  Gdiplus::Color sc0(0,0,0,0); int sn0=1; pb.SetSurroundColors(&sc0,&sn0);
  gg.FillRectangle(&pb,0,0,240,240);
 }
 int sx=GetSystemMetrics(SM_CXSCREEN), sy=GetSystemMetrics(SM_CYSCREEN);
 HWND hw=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,L"ZwSplash",L"ZenWare",WS_POPUP|WS_VISIBLE,(sx-SPL_W)/2,(sy-SPL_H)/2,SPL_W,SPL_H,nullptr,nullptr,hi,nullptr);
 if(hw){
  InitFonts(hw);
  timeBeginPeriod(1);
  int nFrames=0; ULONGLONG msPaint=0; const ULONGLONG tFps0=GetTickCount64();
  for(;;){ // vsync-цикл вместо таймера: кадр рисуется синхронно и ждёт вертикалку
   MSG m{};
   while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){ TranslateMessage(&m); DispatchMessageW(&m); }
   if(!IsWindow(hw)) break;
   if(GetTickCount64()-g_splashT0>=SPLASH_MS){ DestroyWindow(hw); break; }
   ULONGLONG p0=GetTickCount64();
   RedrawWindow(hw,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_NOCHILDREN);
   msPaint+=GetTickCount64()-p0;
   nFrames++;
   if(FAILED(DwmFlush())) Sleep(16);
  }
  timeEndPeriod(1);
  { // диагностика: средний fps и стоимость кадра в файл
   wchar_t fp[MAX_PATH]={}; GetTempPathW(MAX_PATH,fp); wcscat_s(fp,L"ZenWare.splash_fps.txt");
   char buf[256]={}; sprintf_s(buf,"frames=%d totalMs=%llu paintAvgMs=%.2f qBgPart=%.2f qLogo=%.2f qTitle=%.2f qBar=%.2f qBlt=%.2f\r\n",nFrames,GetTickCount64()-tFps0,nFrames?(double)msPaint/nFrames:0.0,nFrames?(double)g_splashQ[0]/nFrames:0.0,nFrames?(double)g_splashQ[1]/nFrames:0.0,g_nQ2?(double)g_splashQ[2]/g_nQ2:0.0,nFrames?(double)g_splashQ[3]/nFrames:0.0,nFrames?(double)g_splashQ[4]/nFrames:0.0);
   HANDLE hf=CreateFileW(fp,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
   if(hf!=INVALID_HANDLE_VALUE){ DWORD wr=0; WriteFile(hf,buf,(DWORD)strlen(buf),&wr,nullptr); CloseHandle(hf); }
  }
 }
 if(g_logoBase){ delete g_logoBase; g_logoBase=nullptr; }
 if(g_splashImg){ delete g_splashImg; g_splashImg=nullptr; }
 if(g_splashGlow){ delete g_splashGlow; g_splashGlow=nullptr; }
 Gdiplus::GdiplusShutdown(tok);
}
} // namespace
LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
  case WM_CREATE:{
   g_hMain=h; InitFonts(h); RefreshTheme();
   SetLayeredWindowAttributes(h,0,1,LWA_ALPHA); // старт почти прозрачным для fade-in
   g_ullLastTick=GetTickCount64();
   CreateWindowExW(0,L"BUTTON",LoaderUtil::SW(L"ЗАПУСТИТЬ ИГРУ",L"LAUNCH GAME"),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,20,92,580,36,h,(HMENU)IDC_LAUNCH,nullptr,nullptr);
   g_hInject=CreateWindowExW(0,L"BUTTON",LoaderUtil::SW(g_bExternal?L"ЗАПУСК EXTERNAL":L"ИНЖЕКТ",g_bExternal?L"LAUNCH EXTERNAL":L"INJECT"),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,20,136,580,52,h,(HMENU)IDC_INJECT,nullptr,nullptr);
   g_hStatus=CreateWindowExW(0,L"STATIC",LoaderUtil::SW(L"Готов",L"Ready"),WS_CHILD|WS_VISIBLE,44,204,556,20,h,(HMENU)IDC_STATUS,nullptr,nullptr);
    SendMessageW(g_hStatus,WM_SETFONT,(WPARAM)g_fUI,TRUE);
   // Autopoisk DLL moved into FindDll()
#if 0
  {
   wchar_t szFound[MAX_PATH] = { };
   wchar_t szExeDir[MAX_PATH] = { };
   GetModuleFileNameW(NULL, szExeDir, MAX_PATH);
   wchar_t* pSlash = wcsrchr(szExeDir, L'\\');
   if (pSlash) *pSlash = L'\0';
   const wchar_t* cands[] = { L"\\ZenWare.dll", L"\\..\\ZenWare.DLL\\bin\\Release\\ZenWare.dll" };
   for (auto rel : cands) {
       wchar_t szTry[MAX_PATH] = { };
       wchar_t szFull[MAX_PATH] = { };
       wcscpy_s(szTry, szExeDir);
       wcscat_s(szTry, rel);
       GetFullPathNameW(szTry, MAX_PATH, szFull, nullptr);
       if (GetFileAttributesW(szFull) != INVALID_FILE_ATTRIBUTES) { wcscpy_s(szFound, szFull); break; }
   }
   if (!szFound[0]) {
       const wchar_t* dev = L"C:\\Users\\ilya\\Desktop\\ZenWare.cc\\ZenWare.DLL\\bin\\Release\\ZenWare.dll";
       if (GetFileAttributesW(dev) != INVALID_FILE_ATTRIBUTES) wcscpy_s(szFound, dev);
   }
   if (szFound[0]) {
       char nb[MAX_PATH] = { };
       WideCharToMultiByte(CP_ACP, 0, szFound, -1, nb, MAX_PATH, nullptr, nullptr);
       SetWindowTextA(g_hPath, nb);
   }
  }
   #endif
   ApplyCtrlTheme(); SetTimer(h,1,16,nullptr); break;
 }
  case WM_TIMER:
    if(w==1){
    { // dt-обновление всех плавных значений раз в кадр
     ULONGLONG now=GetTickCount64();
     float dt=g_ullLastTick?((float)(now-g_ullLastTick)/1000.0f):0.016f; if(dt>0.1f)dt=0.1f;
     g_ullLastTick=now;
     // hover кнопок: цель 1/0 по позиции курсора
     POINT cp{0,0}; GetCursorPos(&cp); ScreenToClient(h,&cp);
     auto hovOf=[&](int id){ HWND b=GetDlgItem(h,id); if(!b) return 0.0f; RECT r; GetWindowRect(b,&r); MapWindowPoints(HWND_DESKTOP,h,(LPPOINT)&r,2); return PtInRect(&r,cp)?1.0f:0.0f; };
     float tL=hovOf(IDC_LAUNCH), tI=hovOf(IDC_INJECT);
     bool changed= false;
     float nL=Approach(g_flHovLaunch,tL,dt,14.0f); if(nL!=g_flHovLaunch){ g_flHovLaunch=nL; changed=true; }
     float nI=Approach(g_flHovInject,tI,dt,14.0f); if(nI!=g_flHovInject){ g_flHovInject=nI; changed=true; }
     float nP=Approach(g_flPressMode,0.0f,dt,10.0f); if(nP!=g_flPressMode){ g_flPressMode=nP; changed=true; }
     if(g_flModeT!=g_flModeTarget){
      float nM=Approach(g_flModeT,g_flModeTarget,dt,8.0f);
      if(fabsf(nM-g_flModeTarget)<0.004f) nM=g_flModeTarget;
      g_flModeT=nM; changed=true;
      HWND bi=GetDlgItem(h,IDC_INJECT), bl2=GetDlgItem(h,IDC_LAUNCH);
      if(bi) InvalidateRect(bi,nullptr,FALSE);
      if(bl2) InvalidateRect(bl2,nullptr,FALSE);
     }
     if(g_bFading){
      g_flWinAlpha=Approach(g_flWinAlpha,1.0f,dt,10.0f);
      BYTE a=(BYTE)(g_flWinAlpha*255);
      SetLayeredWindowAttributes(h,0,a,LWA_ALPHA);
      if(g_flWinAlpha>0.985f){ g_flWinAlpha=1.0f; g_bFading=false; SetLayeredWindowAttributes(h,0,255,LWA_ALPHA); }
      changed=true;
     }
     if(changed){ RECT all={0,0,WINDOW_W,WINDOW_H+40}; InvalidateRect(h,&all,FALSE); }
    }
     RECT hdr={0,0,WINDOW_W,76}; InvalidateRect(h,&hdr,FALSE);
    // перерисовка кнопки запуска, чтобы радужная обводка анимировалась вместе с логотипом
    HWND bl=GetDlgItem(h,IDC_LAUNCH); if(bl) InvalidateRect(bl,nullptr,FALSE);
   if(g_busy){ RECT pr={20,262,600,266}; InvalidateRect(h,&pr,FALSE); }
   // следим, появилась ли игра после кнопки запуска
   static int tick=0;
   if(++tick%10==0){
    bool has=LoaderUtil::FindProcessId(L"left4dead2.exe")!=0;
    if(has&&!g_gameSeen){ g_gameSeen=true; LoaderUtil::Status(g_hMain, LoaderUtil::S("Игра запущена","Game running")); }
    if(!has) g_gameSeen=false;
   }
  } break;
 case WM_MOUSEMOVE:{
  POINT pt{GET_X_LPARAM(l),GET_Y_LPARAM(l)}; ClientToScreen(h,&pt);
  HWND ch=ChildWindowFromPointEx(h,pt,CWP_SKIPINVISIBLE);
  int id=ch?GetDlgCtrlID(ch):0;
   for(int b:{IDC_LAUNCH,IDC_INJECT}){ HWND bh=GetDlgItem(h,b);
   RECT r; GetWindowRect(bh,&r); MapWindowPoints(HWND_DESKTOP,h,(POINT*)&r,2);
   InvalidateRect(h,&r,FALSE); }
   (void)id;
   {
    POINT mp{GET_X_LPARAM(l),GET_Y_LPARAM(l)};
    bool mh=PtInRect(&g_rcMode,mp)!=FALSE;
    if(mh!=g_bModeHov){ g_bModeHov=mh; InvalidateRect(h,&g_rcMode,FALSE); }
    if(mh) SetCursor(LoadCursorW(nullptr,IDC_HAND));
   }
    if(id==IDC_INJECT||id==IDC_LAUNCH){
   TRACKMOUSEEVENT tme{sizeof(tme),TME_LEAVE,h,0}; TrackMouseEvent(&tme);
  }
  break;
 }
  case WM_LBUTTONDOWN:{
   int x=GET_X_LPARAM(l), y=GET_Y_LPARAM(l);
   POINT cp{x,y};
    if(PtInRect(&g_rcMode,cp)){ g_flPressMode=1.0f; ToggleMode(); }
   {
    RECT lr={22,8,220,50};
    static DWORD slc=0; static int sln=0;
    if(PtInRect(&lr,cp)){
     DWORD now=GetTickCount();
     if(now-slc<600){ if(++sln>=3){ sln=0;
      MessageBoxW(h,LoaderUtil::SW(L"ZenWare.cc — internal & external, x86.\nТы нашёл секрет #1. А второй вводится с клавиатуры...",L"ZenWare.cc — internal & external, x86.\nYou found secret #1. The second one is typed on the keyboard..."),L"ZenWare",MB_ICONINFORMATION); } }
     else sln=1;
     slc=now;
    }
   }
   break;
  }
  case WM_MOUSELEAVE:{
  RECT rc; GetClientRect(h,&rc); InvalidateRect(h,&rc,FALSE);
  break;
 }
 case WM_ERASEBKGND:
   return 1;
  case WM_PAINT:{
   PAINTSTRUCT ps; HDC hdc=BeginPaint(h,&ps);
   RECT rc; GetClientRect(h,&rc);
   const int bw=(rc.right>1?rc.right:1), bh=(rc.bottom>1?rc.bottom:1);
   HDC mem=CreateCompatibleDC(hdc);
   HBITMAP bmp=CreateCompatibleBitmap(hdc,bw,bh);
   HGDIOBJ oldBmp=SelectObject(mem,bmp);
   HDC dc=mem;
   {
    int gb1=46+(int)(18*sinf((float)(GetTickCount64()%6283)/1000.0f*0.45f));
    COLORREF gbot=Mix2(g_theme.bg,g_theme.ctl,gb1);
    TRIVERTEX gv[2]={{rc.left,rc.top,(COLOR16)(GetRValue(g_theme.bg)<<8),(COLOR16)(GetGValue(g_theme.bg)<<8),(COLOR16)(GetBValue(g_theme.bg)<<8),0},
     {rc.right,rc.bottom,(COLOR16)(GetRValue(gbot)<<8),(COLOR16)(GetGValue(gbot)<<8),(COLOR16)(GetBValue(gbot)<<8),0}};
    GRADIENT_RECT ggr={0,1}; GdiGradientFill(dc,gv,2,&ggr,1,GRADIENT_FILL_RECT_V);
   }
   { // дрейфующая пыль по фону (детерминированная, без состояния)
    float elP=GetTickCount64()/1000.0f;
    float spd=g_bParty?3.0f:1.0f;
    for(int i=0;i<36;i++){
     int px=(int)(i*97+elP*11*spd*(1+i%3))%bw;
     int py=(int)(i*131-elP*7*spd)%bh; if(py<0)py+=bh;
     int pa=g_bParty?60:14+(i*7)%18;
     HBRUSH pb=CreateSolidBrush(Mix2(g_theme.bg,g_theme.dim,pa));
     RECT pr={px,py,px+2,py+2}; FillRect(dc,&pr,pb); DeleteObject(pb);
    }
   }
  // тёмная шапка с тонкой мятной линией снизу
  RECT hdr={0,0,rc.right,66};
  HBRUSH hb=CreateSolidBrush(g_theme.dark?RGB(8,11,10):RGB(228,242,235));
  FillRect(dc,&hdr,hb); DeleteObject(hb);
   HPEN lp=CreatePen(PS_SOLID,2,Acc()); auto ol=SelectObject(dc,lp);
   MoveToEx(dc,0,66,nullptr); LineTo(dc,rc.right,66);
   SelectObject(dc,ol); DeleteObject(lp);
   { // бегущий блик по линии шапки
    float elP=GetTickCount64()/1000.0f;
    int sx=(int)fmodf(elP*170.0f,(float)(rc.right+240))-120;
    HBRUSH sb=CreateSolidBrush(Mix2(Acc(),RGB(255,255,255),150));
    RECT sr={sx,64,sx+90,68}; FillRect(dc,&sr,sb); DeleteObject(sb);
   }
  // радужный логотип
  DrawRgbLogo(dc,22,8);
    // пилюля-кнопка режима справа (с тактильным сжатием при клике)
    int sq=(int)(g_flPressMode*4.0f); // прижимается на 4px
    RECT vr={rc.right-170+sq,16+sq/2,rc.right-20-sq,42-sq/2}; g_rcMode=vr;
    float hovT=g_bModeHov?1.0f:0.0f;
    static float g_flModeHovA=0.0f; g_flModeHovA=hovT; // dt-сглаживание делает WM_TIMER
    COLORREF mfill=Mix2(g_theme.dark?RGB(6,14,11):RGB(255,255,255),Mix2(g_theme.ctl,Acc(),60),(int)(g_flModeHovA*255));
    HBRUSH vb=CreateSolidBrush(mfill); HPEN vp=CreatePen(PS_SOLID,1,Mix2(g_theme.border,Acc(),(int)(g_flModeHovA*255)));
   auto vo1=SelectObject(dc,vb); auto vo2=SelectObject(dc,vp);
   RoundRect(dc,vr.left,vr.top,vr.right,vr.bottom,12,12);
   SelectObject(dc,vo1); SelectObject(dc,vo2); DeleteObject(vb); DeleteObject(vp);
   SelectObject(dc,g_fSmall); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,g_theme.dark?Acc():Acc2());
   DrawTextW(dc,g_bExternal?L"EXTERNAL • x86":L"INTERNAL • x86",-1,&vr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  // подписи секций
  SelectObject(dc,g_fSmall); SetTextColor(dc,g_theme.dim);


  // рамки вокруг полей



  // прогресс-бар инжекта
  if(g_busy){
   RECT trk={20,196,600,200}; HBRUSH tb=CreateSolidBrush(g_theme.ctl); FillRect(dc,&trk,tb); DeleteObject(tb);
   static int px=0; px=(px+7)%(580+120);
   RECT sg={20+px-120,196,20+px,200}; HBRUSH sb=CreateSolidBrush(Acc()); FillRect(dc,&sg,sb); DeleteObject(sb);
   FrameRect(dc,&trk,g_brBorder);
  }
   // статус-точка (пульсирует, пока идёт работа)
   COLORREF dotC=g_dotColor;
   if(g_busy){ float pl=0.5f+0.5f*sinf(GetTickCount64()/130.0f); dotC=Mix2(Acc(),RGB(255,255,255),(int)(pl*90)); }
   else if(dotC==RGB(120,130,124)){ float br=0.5f+0.5f*sinf(GetTickCount64()/900.0f); dotC=Mix2(g_dotColor,Acc(),(int)(br*40)); }
   { // мягкое свечение вокруг точки
    HBRUSH gb=CreateSolidBrush(Mix2(g_theme.bg,dotC,26));
    RECT gr={17,201,37,221}; FillRect(dc,&gr,gb); DeleteObject(gb);
   }
   { // сама точка
    HBRUSH db=CreateSolidBrush(dotC); HPEN dp=CreatePen(PS_SOLID,1,dotC);
    auto od1=SelectObject(dc,db); auto od2=SelectObject(dc,dp);
    Ellipse(dc,22,206,32,216);
    SelectObject(dc,od1); SelectObject(dc,od2); DeleteObject(db); DeleteObject(dp);
   }
  // футер
  SelectObject(dc,g_fSmall); SetTextColor(dc,g_theme.dim);
  RECT fr={20,306,600,326}; DrawTextW(dc,LoaderUtil::SW(L"Только локальный сервер (-insecure) • логи: %TEMP%\\ZenWare.Loader.log",L"Local server only (-insecure) • logs: %TEMP%\\ZenWare.Loader.log"),-1,&fr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,mem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
  SelectObject(mem,oldBmp); DeleteObject(bmp); DeleteDC(mem);
  EndPaint(h,&ps);
  break;
 }
 case WM_CTLCOLORDLG: return (LRESULT)g_brBg;
 case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC: return ColorChild(m,w,l);
 case WM_DRAWITEM: return DrawBtn(l);
 case WM_SETTINGCHANGE: if(l && !lstrcmpiW((LPCWSTR)l,L"ImmersiveColorSet")) RefreshTheme(); break;
 case LoaderUtil::WM_APP_LOADER:{
  auto p=(LoaderUtil::LogPacket_t*)l;
  if(!p) break;
  if(w==LoaderUtil::KIND_STATUS){ SetWindowTextW(g_hStatus,p->wszText); ClassifyStatus(p->wszText); RECT sr={20,260,600,286}; InvalidateRect(h,&sr,FALSE); }
  delete p; break;
 }
 case WM_COMMAND:
  if(HIWORD(w)==BN_CLICKED){
   switch(LOWORD(w)){
         case 0xBEEF: break; // IDC_BROWSE removed
      case IDC_INJECT: if(g_bExternal) LaunchExternal(); else StartInject(); SetFocus(h); break;
     case IDC_LAUNCH: LaunchGame(); SetFocus(h); break;
    }
   }
   else if(HIWORD(w)==EN_SETFOCUS || HIWORD(w)==EN_KILLFOCUS){ RECT pf={20,100,424,134}; InvalidateRect(h,&pf,FALSE); }
  break;
  case WM_KEYDOWN:{
   static const int seq[]={VK_UP,VK_UP,VK_DOWN,VK_DOWN,VK_LEFT,VK_RIGHT,VK_LEFT,VK_RIGHT,'B','A'};
   static int si=0;
   int k=(int)w;
   if(k==seq[si]){ if(++si>=10){ si=0; g_bParty=!g_bParty;
    LoaderUtil::Status(h,LoaderUtil::S(g_bParty?"PARTY MODE включен":"PARTY MODE выключен",g_bParty?"PARTY MODE on":"PARTY MODE off")); } }
   else si=(k==seq[0])?1:0;
   break;
  }
  case WM_DESTROY: PostQuitMessage(0); break;
 default: return DefWindowProcW(h,m,w,l);
 }
 return 0;
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE, PWSTR,int cmd){
 LoaderUtil::g_bRuLang=(PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_RUSSIAN);
 LoaderUtil::InitFileLog();
 WNDCLASSEXW wc{sizeof(wc),CS_HREDRAW|CS_VREDRAW,WndProc,0,0,hi,LoadIconW(hi,MAKEINTRESOURCEW(IDI_MAINICON)),LoadCursorW(nullptr,IDC_ARROW),nullptr,nullptr,L"Zw3Wnd",(HICON)LoadImageW(hi,MAKEINTRESOURCEW(IDI_MAINICON),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0)};
 RegisterClassExW(&wc);
 WNDCLASSEXW ws{sizeof(ws),CS_HREDRAW|CS_VREDRAW,SplashProc,0,0,hi,LoadIconW(hi,MAKEINTRESOURCEW(IDI_MAINICON)),LoadCursorW(nullptr,IDC_ARROW),nullptr,nullptr,L"ZwSplash",nullptr};
 RegisterClassExW(&ws);
 RunSplash(hi);
 RECT rc{0,0,WINDOW_W,WINDOW_H}; AdjustWindowRect(&rc,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,FALSE);
 int ww=rc.right-rc.left, wh=rc.bottom-rc.top;
 // главное окно открывается ровно там же, где был сплэш — бесшовный переход
 int wx=(GetSystemMetrics(SM_CXSCREEN)-ww)/2, wy=(GetSystemMetrics(SM_CYSCREEN)-wh)/2;
 HWND hw=CreateWindowExW(WS_EX_LAYERED,wc.lpszClassName,L"ZenWare.cc Loader v3.2",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN, wx,wy, WINDOW_W, WINDOW_H, nullptr,nullptr,hi,nullptr);
 SetWindowPos(hw,nullptr,0,0,ww,wh,SWP_NOMOVE|SWP_NOZORDER);
 ShowWindow(hw,cmd); UpdateWindow(hw);
 MSG m{}; while(GetMessageW(&m,nullptr,0,0)>0){ TranslateMessage(&m); DispatchMessageW(&m);} return (int)m.wParam;
}
