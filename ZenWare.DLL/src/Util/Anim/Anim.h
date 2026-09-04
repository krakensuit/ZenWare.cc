#pragma once
#include <cmath>
#include <algorithm>
#if __has_include(<imgui.h>)
#include <imgui.h>
#endif

namespace Anim {

inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
#ifdef IMGUI_VERSION
inline ImVec4 Lerp(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(Lerp(a.x,b.x,t), Lerp(a.y,b.y,t), Lerp(a.z,b.z,t), Lerp(a.w,b.w,t));
}
#endif
inline float EaseOutCubic(float t) { return 1.0f - powf(1.0f - t, 3.0f); }
inline float EaseInOut(float t) { return t < 0.5f ? 4*t*t*t : 1 - powf(-2*t+2,3)/2; }

// анимированное значение 0..1 к цели с дельтой времени
inline float Approach(float cur, float target, float dt, float speed = 12.0f) {
    return Lerp(cur, target, 1.0f - expf(-speed * dt));
}

} // namespace Anim
