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
 COLORREF bg, ctl, text, dim, accent, accent2, border;
 bool dark;
};

bool IsSystemDark(){
 DWORD v=0, s=sizeof(v);
 if(RegGetValueA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize","AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&v,&s)==ERROR_SUCCESS) return v==0;
 return true;
}
Theme_t MakeTheme(bool d){
 Theme_t t{}; t.dark=d;
 if(d){ t.bg=RGB(12,14,13); t.ctl=RGB(22,29,25); t.text=RGB(232,255,245); t.dim=RGB(118,142,132); t.accent=RGB(0,255,171); t.accent2=RGB(0,170,113); t.border=RGB(42,60,50); }
 else { t.bg=RGB(242,249,244); t.ctl=RGB(255,255,255); t.text=RGB(20,35,30); t.dim=RGB(100,115,110); t.accent=RGB(0,200,135); t.accent2=RGB(0,150,100); t.border=RGB(178,218,198); }
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
HWND g_hMain=nullptr, g_hInject=nullptr, g_hStatus=nullptr, g_hLaunch=nullptr;
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
static void ClassifyStatus(const wchar_t* t){
 if(!t) return;
 auto has=[](const wchar_t* h,const wchar_t* n){ return wcsstr(h,n)!=nullptr; };
 if(has(t,L"Ошибка")||has(t,L"Error")||has(t,L"FAILED")||has(t,L"не найден")||has(t,L"not found")||has(t,L"[!]"))
  g_dotColor=RGB(255,80,80);
 else if(has(t,L"Готово")||has(t,L"Done")||has(t,L"успешно")||has(t,L"success")||has(t,L"[===]"))
  g_dotColor=RGB(0,255,171);
 else if(g_busy) g_dotColor=g_theme.accent;
}
LRESULT DrawBtn(LPARAM lp){
 auto d=(LPDRAWITEMSTRUCT)lp; if(!d) return TRUE;
 bool en=IsWindowEnabled(d->hwndItem); bool pr=(d->itemState & ODS_SELECTED)!=0; bool pri=(d->CtlID==IDC_INJECT);
 bool hov=(GetCapture()==d->hwndItem)||(d->itemState & ODS_FOCUS && false);
 POINT pt={0,0}; GetCursorPos(&pt); ScreenToClient(d->hwndItem,&pt);
 RECT cr; GetClientRect(d->hwndItem,&cr);
 hov = en && PtInRect(&cr,pt);
 // закрасить всё поле кнопки цветом диалога и обрезать рисование скруглением,
 // иначе по краям остаются неокрашенные белые пиксели
 FillRect(d->hDC,&cr,g_brBg);
 HRGN rgClip=CreateRoundRectRgn(cr.left,cr.top,cr.right+1,cr.bottom+1,12,12);
 SelectClipRgn(d->hDC,rgClip);
 COLORREF fill=g_theme.ctl, txt=g_theme.text, br=Mix2(g_theme.border,g_theme.accent,110);
 if(pri&&en){
  fill=hov?Mix2(g_theme.accent,RGB(255,255,255),50):g_theme.accent; txt=g_theme.dark?RGB(4,12,8):RGB(255,255,255); br=g_theme.accent;
  // вертикальный градиент поверх заливки
  TRIVERTEX vv[2]={{cr.left,cr.top,(COLOR16)(GetRValue(fill)<<8),(COLOR16)(GetGValue(fill)<<8),(COLOR16)(GetBValue(fill)<<8),0},
   {cr.right,cr.bottom,(COLOR16)(GetRValue(g_theme.accent2)<<8),(COLOR16)(GetGValue(g_theme.accent2)<<8),(COLOR16)(GetBValue(g_theme.accent2)<<8),0}};
  GRADIENT_RECT gr={0,1}; GdiGradientFill(d->hDC,vv,2,&gr,1,GRADIENT_FILL_RECT_V);
 } else {
   if(!en){ fill=g_theme.bg; txt=g_theme.dim; }
   else if(pr){ fill=Mix2(g_theme.ctl,g_theme.accent,60); }
   else if(hov){ fill=Mix2(g_theme.ctl,g_theme.accent,36); br=g_theme.accent; }
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
  HPEN bp=CreatePen(PS_SOLID,1,g_theme.accent); auto ob=SelectObject(d->hDC,bp);
  HGDIOBJ nb=SelectObject(d->hDC,GetStockObject(NULL_BRUSH));
  RECT r=cr; InflateRect(&r,-1,-1); RoundRect(d->hDC,r.left,r.top,r.right,r.bottom,10,10);
  SelectObject(d->hDC,nb); SelectObject(d->hDC,ob); DeleteObject(bp);
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
 SetTextColor(dc, id==IDC_STATUS?g_theme.accent:g_theme.dim); SetBkColor(dc,g_theme.bg); return (LRESULT)g_brBg;
}
// ---------- кодовый интро-сплэш: безрамочное окно ~2.8с, клик пропускает ----------
static constexpr int SPL_W=560, SPL_H=300;
static constexpr ULONGLONG SPLASH_MS=2800;
static HWND g_hSplash=nullptr;
static ULONGLONG g_splashT0=0;
struct SplashP_t{ float x0,y0,x1,y1,dl; int sz; };
static SplashP_t g_parts[70];
static Gdiplus::Image* g_splashImg=nullptr;
static unsigned SplashRnd(unsigned& s){ s^=s<<13; s^=s>>17; s^=s<<5; return s; }
static float SplashEase(float t){ if(t<0) t=0; if(t>1) t=1; return t*t*(3.0f-2.0f*t); }
LRESULT CALLBACK SplashProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:{
  g_hSplash=h; g_splashT0=GetTickCount64();
  unsigned s=(unsigned)GetTickCount64()|1u;
  for(int i=0;i<70;i++){
   int e=SplashRnd(s)%4; float ex,ey;
   if(e==0){ ex=(float)(SplashRnd(s)%SPL_W); ey=-12; }
   else if(e==1){ ex=(float)(SplashRnd(s)%SPL_W); ey=SPL_H+12; }
   else if(e==2){ ex=-12; ey=(float)(SplashRnd(s)%SPL_H); }
   else { ex=SPL_W+12; ey=(float)(SplashRnd(s)%SPL_H); }
   g_parts[i]={ex,ey,(float)(SPL_W/2+(int)(SplashRnd(s)%170)-85),(float)(150+(int)(SplashRnd(s)%90)-45),(SplashRnd(s)%600)/1000.0f,2+(int)(SplashRnd(s)%2)};
  }
  SetTimer(h,1,30,nullptr);
  SetLayeredWindowAttributes(h,0,0,LWA_ALPHA);
  break;
 }
 case WM_TIMER:{
  ULONGLONG el=GetTickCount64()-g_splashT0;
  BYTE a=255;
  if(el<300) a=(BYTE)(el*255/300);
  else if(el>SPLASH_MS-400) a=(el>=SPLASH_MS)?0:(BYTE)((SPLASH_MS-el)*255/400);
  SetLayeredWindowAttributes(h,0,a,LWA_ALPHA);
  InvalidateRect(h,nullptr,FALSE);
  if(el>=SPLASH_MS){ KillTimer(h,1); DestroyWindow(h); }
  break;
 }
 case WM_PAINT:{
  PAINTSTRUCT ps; HDC hdc=BeginPaint(h,&ps);
  HDC mem=CreateCompatibleDC(hdc);
  HBITMAP bmp=CreateCompatibleBitmap(hdc,SPL_W,SPL_H);
  HGDIOBJ obm=SelectObject(mem,bmp);
  HDC dc=mem;
  const COLORREF bg=RGB(8,10,9);
  HBRUSH bb=CreateSolidBrush(bg); RECT rc={0,0,SPL_W,SPL_H}; FillRect(dc,&rc,bb); DeleteObject(bb);
  const float el=(GetTickCount64()-g_splashT0)/1000.0f;
  const float hue=fmodf((float)GetTickCount64()/38.0f,360.0f);
  // частицы слетаются к центру
  for(int i=0;i<70;i++){
   const SplashP_t& p=g_parts[i];
   float t=SplashEase((el-p.dl)/1.1f);
   if(t<=0||t>=1) continue;
   float px=p.x0+(p.x1-p.x0)*t, py=p.y0+(p.y1-p.y0)*t;
   COLORREF c=Mix2(bg,Hsv(hue+i*3.0f,0.85f,1.0f),(int)(220*(1.0f-t)+35));
   HBRUSH pb=CreateSolidBrush(c);
   RECT pr={(int)px-p.sz/2,(int)py-p.sz/2,(int)px+p.sz/2+1,(int)py+p.sz/2+1};
   FillRect(dc,&pr,pb); DeleteObject(pb);
  }
   // эмблема: настоящий логотип со свечением, ESP-уголками и сканлайном
   float la=SplashEase((el-0.15f)/0.6f);
   const int LCX=SPL_W/2, LCY=89, LSZ=150;
   if(la>0){
    if(g_splashImg){
     Gdiplus::Graphics gd(dc);
     gd.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
     gd.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
     int gr=LSZ/2+34+(int)(10*sinf(el*4.0f));
     Gdiplus::GraphicsPath gp; gp.AddEllipse(LCX-gr,LCY-gr,gr*2,gr*2);
     Gdiplus::PathGradientBrush pb(&gp);
     int ga=46+(int)(26*sinf(el*4.0f)); if(ga<0)ga=0; if(ga>120)ga=120;
     pb.SetCenterColor(Gdiplus::Color((BYTE)ga,0,255,171));
     Gdiplus::Color sc(0,0,0,0); int sn=1; pb.SetSurroundColors(&sc,&sn);
     gd.FillEllipse(&pb,LCX-gr,LCY-gr,gr*2,gr*2);
     float sc2=0.75f+0.25f*SplashEase(el/0.9f);
     int L=(int)(LSZ*sc2);
     Gdiplus::ColorMatrix cm={1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,la,0, 0,0,0,0,1};
     Gdiplus::ImageAttributes at; at.SetColorMatrix(&cm,Gdiplus::ColorMatrixFlagsDefault,Gdiplus::ColorAdjustTypeBitmap);
     int iw=(int)g_splashImg->GetWidth(), ih=(int)g_splashImg->GetHeight();
     gd.DrawImage(g_splashImg,Gdiplus::Rect(LCX-L/2,LCY-L/2,L,L),0,0,iw,ih,Gdiplus::UnitPixel,&at);
     float bt=SplashEase((el-0.5f)/0.7f);
     if(bt>0){
      int mg=14+(int)(40*(1.0f-bt));
      int bx0=LCX-LSZ/2-mg, bx1=LCX+LSZ/2+mg, by0=LCY-LSZ/2-mg, by1=LCY+LSZ/2+mg;
      int bl=30, bk=(int)(bt*255);
      HPEN hp=CreatePen(PS_SOLID,2,Mix2(bg,g_theme.accent,bk)); auto oh=SelectObject(dc,hp);
      MoveToEx(dc,bx0,by0+bl,nullptr); LineTo(dc,bx0,by0); LineTo(dc,bx0+bl,by0);
      MoveToEx(dc,bx1-bl,by0,nullptr); LineTo(dc,bx1,by0); LineTo(dc,bx1,by0+bl);
      MoveToEx(dc,bx1,by1-bl,nullptr); LineTo(dc,bx1,by1); LineTo(dc,bx1-bl,by1);
      MoveToEx(dc,bx0+bl,by1,nullptr); LineTo(dc,bx0,by1); LineTo(dc,bx0,by1-bl);
      SelectObject(dc,oh); DeleteObject(hp);
     }
     if(el>0.7f&&el<1.9f){
      int sy2=(LCY-LSZ/2)+(int)(LSZ*((el-0.7f)/1.2f));
      HBRUSH sb=CreateSolidBrush(Mix2(bg,g_theme.accent,(int)(la*90))); RECT sr={LCX-LSZ/2,sy2,LCX+LSZ/2,sy2+2}; FillRect(dc,&sr,sb); DeleteObject(sb);
     }
    }
    const wchar_t* txt=L"ZenWare.cc";
   auto of=SelectObject(dc,g_fTitle); SetBkMode(dc,TRANSPARENT);
   SIZE sz={0,0}; GetTextExtentPoint32W(dc,txt,(int)wcslen(txt),&sz);
    int x0=(SPL_W-sz.cx)/2, y0=g_splashImg?172:104;
   int k=(int)(la*255);
   for(int dx=-2;dx<=2;dx+=2) for(int dy=-2;dy<=2;dy+=2){
    if(!dx&&!dy) continue;
    SetTextColor(dc,Mix2(bg,Hsv(hue,0.9f,0.35f),k));
    TextOutW(dc,x0+dx,y0+dy,txt,(int)wcslen(txt));
   }
   int cx=x0;
   for(const wchar_t* p=txt;*p;++p){
    int idx=(int)(p-txt);
    SetTextColor(dc,Mix2(bg,Hsv(hue+idx*5.0f,0.85f,1.0f),k));
    wchar_t ch[2]={*p,0}; SIZE cs={0,0}; GetTextExtentPoint32W(dc,ch,1,&cs);
    TextOutW(dc,cx,y0,ch,1); cx+=cs.cx;
   }
   // раскрывающаяся линия + подпись
   float lw=SplashEase((el-0.9f)/0.8f);
   if(lw>0){
     int hw2=(int)(200*lw);
     int ly=g_splashImg?214:196;
     HPEN lp=CreatePen(PS_SOLID,2,Mix2(bg,g_theme.accent,(int)(la*255))); auto ol=SelectObject(dc,lp);
     MoveToEx(dc,SPL_W/2-hw2,ly,nullptr); LineTo(dc,SPL_W/2+hw2,ly);
    SelectObject(dc,ol); DeleteObject(lp);
    auto os=SelectObject(dc,g_fSmall);
    SetTextColor(dc,Mix2(bg,g_theme.dim,(int)(la*255)));
     RECT vr={0,ly+8,SPL_W,ly+28}; DrawTextW(dc,L"LOADER v3.2  •  EXTERNAL x86",-1,&vr,DT_CENTER|DT_SINGLELINE);
    SelectObject(dc,os);
   }
   SelectObject(dc,of);
  }
  // прогресс-бар
  {
   float f=el/2.6f; if(f>1) f=1;
   RECT tr={80,252,480,256}; HBRUSH tb=CreateSolidBrush(g_theme.ctl); FillRect(dc,&tr,tb); DeleteObject(tb);
   if(f>0){ RECT fl={80,252,80+(int)(400*f),256}; HBRUSH fb=CreateSolidBrush(g_theme.accent); FillRect(dc,&fl,fb); DeleteObject(fb); }
  }
  auto os2=SelectObject(dc,g_fSmall); SetTextColor(dc,g_theme.dim);
  RECT hr={0,272,SPL_W-16,292}; DrawTextW(dc,LoaderUtil::SW(L"клик — пропустить",L"click to skip"),-1,&hr,DT_RIGHT|DT_SINGLELINE);
  SelectObject(dc,os2);
  BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,mem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
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
 int sx=GetSystemMetrics(SM_CXSCREEN), sy=GetSystemMetrics(SM_CYSCREEN);
 HWND hw=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,L"ZwSplash",L"ZenWare",WS_POPUP|WS_VISIBLE,(sx-SPL_W)/2,(sy-SPL_H)/2,SPL_W,SPL_H,nullptr,nullptr,hi,nullptr);
 if(hw){
  InitFonts(hw);
  MSG m{};
  while(IsWindow(hw)&&GetMessageW(&m,nullptr,0,0)>0){ TranslateMessage(&m); DispatchMessageW(&m); }
 }
 if(g_splashImg){ delete g_splashImg; g_splashImg=nullptr; }
 Gdiplus::GdiplusShutdown(tok);
}
} // namespace
LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:{
  g_hMain=h; InitFonts(h); RefreshTheme();
   CreateWindowExW(0,L"BUTTON",LoaderUtil::SW(L"ЗАПУСТИТЬ ИГРУ",L"LAUNCH GAME"),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,20,92,580,36,h,(HMENU)IDC_LAUNCH,nullptr,nullptr);
   g_hInject=CreateWindowExW(0,L"BUTTON",LoaderUtil::SW(L"ИНЖЕКТ",L"INJECT"),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,20,136,580,52,h,(HMENU)IDC_INJECT,nullptr,nullptr);
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
   ApplyCtrlTheme(); SetTimer(h,1,30,nullptr); break;
 }
 case WM_TIMER:
  if(w==1){
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
   if(id==IDC_INJECT||id==IDC_LAUNCH){
   TRACKMOUSEEVENT tme{sizeof(tme),TME_LEAVE,h,0}; TrackMouseEvent(&tme);
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
   HDC dc=mem; FillRect(dc,&rc,g_brBg);
  // тёмная шапка с тонкой мятной линией снизу
  RECT hdr={0,0,rc.right,66};
  HBRUSH hb=CreateSolidBrush(g_theme.dark?RGB(8,11,10):RGB(228,242,235));
  FillRect(dc,&hdr,hb); DeleteObject(hb);
  HPEN lp=CreatePen(PS_SOLID,1,g_theme.accent); auto ol=SelectObject(dc,lp);
  MoveToEx(dc,0,66,nullptr); LineTo(dc,rc.right,66);
  SelectObject(dc,ol); DeleteObject(lp);
  // радужный логотип
  DrawRgbLogo(dc,22,8);
  // пилюля справа
  RECT vr={rc.right-170,16,rc.right-20,42};
  HBRUSH vb=CreateSolidBrush(g_theme.dark?RGB(6,14,11):RGB(255,255,255)); HPEN vp=CreatePen(PS_SOLID,1,g_theme.border);
  auto vo1=SelectObject(dc,vb); auto vo2=SelectObject(dc,vp);
  RoundRect(dc,vr.left,vr.top,vr.right,vr.bottom,12,12);
  SelectObject(dc,vo1); SelectObject(dc,vo2); DeleteObject(vb); DeleteObject(vp);
  SelectObject(dc,g_fSmall); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,g_theme.dark?g_theme.accent:g_theme.accent2);
  DrawTextW(dc,L"EXTERNAL • x86",-1,&vr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  // подписи секций
  SelectObject(dc,g_fSmall); SetTextColor(dc,g_theme.dim);


  // рамки вокруг полей



  // прогресс-бар инжекта
  if(g_busy){
   RECT trk={20,196,600,200}; HBRUSH tb=CreateSolidBrush(g_theme.ctl); FillRect(dc,&trk,tb); DeleteObject(tb);
   static int px=0; px=(px+7)%(580+120);
   RECT sg={20+px-120,196,20+px,200}; HBRUSH sb=CreateSolidBrush(g_theme.accent); FillRect(dc,&sg,sb); DeleteObject(sb);
   FrameRect(dc,&trk,g_brBorder);
  }
  // статус-точка
  HBRUSH db=CreateSolidBrush(g_dotColor); HPEN dp=CreatePen(PS_SOLID,1,g_dotColor);
  auto od1=SelectObject(dc,db); auto od2=SelectObject(dc,dp);
   Ellipse(dc,22,206,32,216);
  SelectObject(dc,od1); SelectObject(dc,od2); DeleteObject(db); DeleteObject(dp);
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
     case IDC_INJECT: StartInject(); SetFocus(h); break;
     case IDC_LAUNCH: LaunchGame(); SetFocus(h); break;
    }
   }
   else if(HIWORD(w)==EN_SETFOCUS || HIWORD(w)==EN_KILLFOCUS){ RECT pf={20,100,424,134}; InvalidateRect(h,&pf,FALSE); }
  break;
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
 HWND hw=CreateWindowExW(0,wc.lpszClassName,L"ZenWare.cc Loader v3.2",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN, wx,wy, WINDOW_W, WINDOW_H, nullptr,nullptr,hi,nullptr);
 SetWindowPos(hw,nullptr,0,0,ww,wh,SWP_NOMOVE|SWP_NOZORDER);
 ShowWindow(hw,cmd); UpdateWindow(hw);
 MSG m{}; while(GetMessageW(&m,nullptr,0,0)>0){ TranslateMessage(&m); DispatchMessageW(&m);} return (int)m.wParam;
}
