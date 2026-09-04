#include "ZenWareStyle.h"
#include "../../external/Icons/IconsFontAwesome6.h"
#include "../Util/Anim/Anim.h"
#include <unordered_map>

void ApplyZenWareStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 2.0f; s.FrameRounding = 2.0f; s.GrabRounding = 2.0f;
    s.WindowBorderSize = 1.0f; s.FrameBorderSize = 1.0f;
    s.WindowPadding = ImVec2(12,12); s.FramePadding = ImVec2(10,6);
    s.ItemSpacing = ImVec2(8,8);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f,0.06f,0.07f,0.98f);
    c[ImGuiCol_ChildBg] = ImVec4(0.09f,0.09f,0.12f,1.00f);
    c[ImGuiCol_Border] = ImVec4(0.00f,1.00f,0.67f,0.50f);
    c[ImGuiCol_Text] = ImVec4(1,1,1,1); c[ImGuiCol_TextDisabled]=ImVec4(0.69f,0.69f,0.75f,1);
    c[ImGuiCol_Button]=ImVec4(0.00f,0.70f,0.45f,1);
    c[ImGuiCol_ButtonHovered]=ImVec4(0.20f,1.00f,0.73f,1);
    c[ImGuiCol_ButtonActive]=ImVec4(0.00f,1.00f,0.67f,1);
    c[ImGuiCol_FrameBg]=ImVec4(0.10f,0.10f,0.14f,1); c[ImGuiCol_FrameBgHovered]=ImVec4(0.12f,0.12f,0.16f,1);
    c[ImGuiCol_CheckMark]=ImVec4(0,1,0.67f,1); c[ImGuiCol_SliderGrab]=ImVec4(0,1,0.67f,1);
    c[ImGuiCol_Header]=ImVec4(0,1,0.67f,1); c[ImGuiCol_HeaderHovered]=ImVec4(0.2f,1,0.73f,1);
}

void DrawZenWareLogo(ImDrawList* dl, ImVec2 pos, float scale) {
    const char* txt="ZenWare";
    ImFont* f = ImGui::GetFont(); float sz = f->FontSize * scale;
    for(int i=3;i>0;--i) dl->AddText(f, sz, pos+ImVec2(i,i), IM_COL32(0,255,171, 25*i), txt);
    ImVec2 p = pos;
    for(const char* c=txt; *c; ++c){
        float t = (p.x-pos.x)/ (f->FontSize*4.2f);
        ImVec4 col = Anim::Lerp(ImVec4(0,1,0.67f,1), ImVec4(0.2f,1,0.73f,1), t);
        char b[2]={*c,0}; dl->AddText(f, sz, p, ImGui::ColorConvertFloat4ToU32(col), b);
        p.x += f->FontSize*0.62f;
    }
}

namespace ZenWare {
bool Button(const char* label, ImVec2 size){
    ImGuiID id = ImGui::GetID(label);
    static std::unordered_map<ImGuiID,float> hov;
    float &t = hov[id];
    bool h = ImGui::IsItemHovered();
    t = Anim::Approach(t, h?1:0, ImGui::GetIO().DeltaTime, 12.0f);
    ImVec4 col = Anim::Lerp(ImVec4(0,0.70f,0.45f,1), ImVec4(0.2f,1,0.73f,1), t);
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    bool ret = ImGui::Button(label, size);
    ImGui::PopStyleColor();
    return ret;
}
bool Toggle(const char* label, bool* v){
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool h = ImGui::IsItemHovered();
    static std::unordered_map<ImGuiID,float> anim;
    float &t = anim[ImGui::GetID(label)];
    t = Anim::Approach(t, *v?1:0, ImGui::GetIO().DeltaTime, 10.0f);
    ImGui::InvisibleButton(label, ImVec2(36,18));
    if(ImGui::IsItemClicked()) *v ^=1;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, p+ImVec2(36,18), IM_COL32(22,22,31,255), 9.0f);
    ImU32 col = ImGui::ColorConvertFloat4ToU32(Anim::Lerp(ImVec4(0.3f,0.3f,0.3f,1), ImVec4(0,1,0.67f,1), t));
    dl->AddRectFilled(p+ImVec2(1,1), p+ImVec2(35,17), col, 9.0f);
    dl->AddCircleFilled(p+ImVec2(9+18*t,9), 7, IM_COL32(255,255,255,255));
    ImGui::SameLine(); ImGui::TextUnformatted(label);
    return *v;
}
void Tab(const char* label, int idx, int* active){
    bool sel = *active==idx;
    if(sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,1,0.67f,1));
    else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06f,0.06f,0.07f,1));
    if(ImGui::Button(label, ImVec2(90,26))) *active=idx;
    ImGui::PopStyleColor();
}
}
