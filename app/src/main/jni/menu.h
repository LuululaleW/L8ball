#pragma once
#include "include/includes.h"
#include "game.h"
#include "game/Ruleset.h"
#include "imgui/inc/8bp.h"
#include "oxorany/oxorany.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <sys/system_properties.h>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <chrono>
#include <map>
#include <string>
#include <thread>
#include <Vector/Vectors.h>
#include <imgui/imgui.h>
#include "icons/icons.h"
#include "game/inc/AutoPlay.h"

using namespace ImGui;
using namespace std;

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef ImGuiWindowFlags_NoBackground
#define ImGuiWindowFlags_NoBackground 0
#endif

// ====================================================================================
// COLORS
// ====================================================================================

#define BLUE_BRIGHT   IM_COL32(60, 180, 255, 255)
#define BLUE_MAIN     IM_COL32(30, 120, 220, 255)
#define BLUE_DIM      IM_COL32(20, 70, 150, 255)
#define BLUE_GLOW     IM_COL32(60, 180, 255, 80)

#define RED_BRIGHT    IM_COL32(255, 60, 60, 255)
#define RED_MAIN      IM_COL32(220, 30, 30, 255)
#define RED_DIM       IM_COL32(150, 20, 20, 255)
#define RED_GLOW      IM_COL32(255, 60, 60, 80)
#define GREEN_GLOW    IM_COL32(0, 255, 0, 80)

#define BLACK_DEEP    IM_COL32(6, 8, 16, 255)
#define BLACK_CARD    IM_COL32(10, 14, 24, 255)
#define BLACK_PANEL   IM_COL32(14, 18, 30, 255)
#define BLACK_ITEM    IM_COL32(22, 26, 40, 255)

#define GREY_TEXT     IM_COL32(180, 190, 220, 255)
#define WHITE_TEXT    IM_COL32(255, 255, 255, 255)
#define GOLD_BRIGHT   IM_COL32(255, 210, 60, 255)

#define CYAN_BRIGHT   IM_COL32(0, 255, 255, 255)
#define PURPLE_BRIGHT IM_COL32(200, 100, 255, 255)
#define ORANGE_BRIGHT IM_COL32(255, 165, 0, 255)
#define PINK_BRIGHT   IM_COL32(255, 50, 150, 255)

// ✅ Forward declarations for AutoPlay helpers


static const ImVec4 BLUE_VEC4      = ImVec4(0.12f, 0.47f, 0.86f, 1.0f);
static const ImVec4 BLUE_DIM_VEC4  = ImVec4(0.08f, 0.27f, 0.55f, 1.0f);
static const ImVec4 RED_VEC4       = ImVec4(0.86f, 0.12f, 0.12f, 1.0f);
static const ImVec4 RED_DIM_VEC4   = ImVec4(0.55f, 0.08f, 0.08f, 1.0f);
static const ImVec4 GREY_BLUE_VEC4 = ImVec4(0.63f, 0.70f, 0.82f, 1.0f);
static const ImVec4 GOLD_VEC4      = ImVec4(0.83f, 0.67f, 0.16f, 1.0f);
static const ImVec4 GREEN_VEC4     = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static const ImVec4 WHITE_VEC4     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 CYAN_VEC4      = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 PURPLE_VEC4    = ImVec4(0.78f, 0.39f, 1.0f, 1.0f);
static const ImVec4 ORANGE_VEC4    = ImVec4(1.0f, 0.65f, 0.0f, 1.0f);
static const ImVec4 PINK_VEC4      = ImVec4(1.0f, 0.20f, 0.59f, 1.0f);

static const ImVec4 SLIDER_BLUE_VEC4       = ImVec4(0.31f, 0.55f, 1.0f, 1.0f);
static const ImVec4 SLIDER_BLUE_ACTIVE_VEC4 = ImVec4(0.47f, 0.71f, 1.0f, 1.0f);
static const ImVec4 SLIDER_PINK_VEC4       = ImVec4(0.86f, 0.31f, 0.71f, 1.0f);
static const ImVec4 SLIDER_PINK_ACTIVE_VEC4 = ImVec4(1.0f, 0.47f, 0.86f, 1.0f);

struct MenuState {
    bool isOpen = false;
    int currentTab = 0;
    float sidebarWidth = 950.0f;
    float animProgress = 0.0f;
    float menuAlpha = 0.0f;
    float menuScale = 0.9f;
    float glowTime = 0.0f;
    float pulseTime = 0.0f;
    float slideOffset = 0.0f;
    float waveTime = 0.0f;
    ImVec4 accentColor = ImVec4(0.12f, 0.47f, 0.86f, 1.0f);
};

static MenuState g_menu;

// ====================================================================================
// GLOBALS
// ====================================================================================

static float g_sideBtnsY = 0.0f;
float g_toggleRotAngle = 0.0f;
bool g_autoPlayCalculating = false;
static float g_pocketGlowTime = 0.0f;
static float g_saveNotifTimer = 0.0f;

// ✅ Throttling timers
static float g_lastAutoPlayUpdate = 0.0f;
static float g_lastSaveTime = 0.0f;
static float g_lastInGameCheck = 0.0f;
static bool  g_isInGameCached = false;
static bool  g_userInfoPatched = false;

// ====================================================================================
// Forward Declarations
// ====================================================================================

static bool ToggleSwitch(const char* label, bool* v, float screenScale);
static void DrawFloatingButton(ImGuiIO& io);
static void DrawMenu(ImGuiIO& io);
static void SetupImgui();

// ====================================================================================
// Pulse
// ====================================================================================

static float GetPulseScale(float time) {
    return 1.0f + 0.04f * sinf(time * 2.0f);
}

// ====================================================================================
// Pocket glow
// ====================================================================================

static void DrawPocketGlowEffect(ImDrawList* dl, ImVec2 screenPos, float radius, float time, ImU32 color) {
    int cr = (color >> 16) & 0xFF;
    int cg = (color >> 8) & 0xFF;
    int cb = color & 0xFF;

    for (int i = 8; i > 0; i--) {
        float r = radius * (0.25f + i * 0.12f);
        int alpha = (int)(80 * (1.0f - i / 8.0f) * (0.6f + 0.4f * sinf(time * 2.5f)));
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        dl->AddCircleFilled(screenPos, r, IM_COL32(cr, cg, cb, alpha));
    }
    
    for (int i = 0; i < 4; i++) {
        float phase = fmodf(time * 1.4f + i * 0.25f, 1.0f);
        float ringR = radius * (0.5f + phase * 1.1f);
        int alpha = (int)(220 * (1.0f - phase) * (1.0f - phase));
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        dl->AddCircle(screenPos, ringR, IM_COL32(cr, cg, cb, alpha), 64, 3.5f - i * 0.6f);
    }
    
    int numDash = 24;
    float dashSpeed = time * 1.2f;
    for (int i = 0; i < numDash; i++) {
        float a1 = dashSpeed + (i * 2.0f * PI / numDash);
        float a2 = a1 + (2.0f * PI / numDash) * 0.45f;
        float ringR = radius * 1.05f;
        ImVec2 p1(screenPos.x + cosf(a1) * ringR, screenPos.y + sinf(a1) * ringR);
        ImVec2 p2(screenPos.x + cosf(a2) * ringR, screenPos.y + sinf(a2) * ringR);
        int alpha = (int)(180 + 75 * sinf(time * 3.0f + i * 0.3f));
        if (alpha > 255) alpha = 255;
        if (alpha < 0) alpha = 0;
        dl->AddLine(p1, p2, IM_COL32(cr, cg, cb, alpha), 2.5f);
    }
    
    int numRays = 14;
    float baseAngle = time * 0.9f;
    for (int i = 0; i < numRays; i++) {
        float angle = baseAngle + (i * 2.0f * PI / numRays);
        float innerR = radius * 0.85f;
        float outerR = radius * (1.35f + 0.2f * sinf(time * 3.5f + i * 0.6f));
        
        ImVec2 p1(screenPos.x + cosf(angle) * innerR, screenPos.y + sinf(angle) * innerR);
        ImVec2 p2(screenPos.x + cosf(angle) * outerR, screenPos.y + sinf(angle) * outerR);
        
        int alpha = (int)(160 + 95 * sinf(time * 3.0f + i * 0.8f));
        if (alpha > 255) alpha = 255;
        if (alpha < 0) alpha = 0;
        
        dl->AddLine(p1, p2, IM_COL32(cr, cg, cb, alpha), 2.5f);
    }
    
    int numDots = 20;
    for (int i = 0; i < numDots; i++) {
        float angle = -time * 0.6f + (i * 2.0f * PI / numDots);
        float dotR = radius * (1.15f + 0.25f * sinf(time * 1.8f + i * 0.5f));
        float dotX = screenPos.x + cosf(angle) * dotR;
        float dotY = screenPos.y + sinf(angle) * dotR;
        
        float size = 2.0f + 3.0f * (0.5f + 0.5f * sinf(time * 3.0f + i * 0.7f));
        int alpha = (int)(200 + 55 * sinf(time * 3.5f + i * 0.9f));
        if (alpha > 255) alpha = 255;
        if (alpha < 0) alpha = 0;
        
        dl->AddCircleFilled(ImVec2(dotX, dotY), size, IM_COL32(cr, cg, cb, alpha));
    }
    
    float innerPulse = 1.0f + 0.15f * sinf(time * 4.5f);
    dl->AddCircleFilled(screenPos, radius * 0.55f * innerPulse, IM_COL32(cr, cg, cb, 60));
    dl->AddCircle(screenPos, radius * 0.7f * innerPulse, IM_COL32(255, 100, 100, 180), 64, 2.5f);
    dl->AddCircle(screenPos, radius * 0.75f * innerPulse, IM_COL32(255, 60, 60, 120), 64, 1.5f);
    
    for (int i = 0; i < 7; i++) {
        float angle = time * 2.0f + (i * 2.0f * PI / 7.0f);
        float dotR = radius * 0.4f;
        float dotX = screenPos.x + cosf(angle) * dotR;
        float dotY = screenPos.y + sinf(angle) * dotR;
        int alpha = (int)(150 + 105 * sinf(time * 5.0f + i));
        if (alpha > 255) alpha = 255;
        if (alpha < 0) alpha = 0;
        dl->AddCircleFilled(ImVec2(dotX, dotY), 3.5f, IM_COL32(255, 220, 220, alpha));
    }
    
    float outerGlow = 1.0f + 0.08f * sinf(time * 2.0f);
    dl->AddCircle(screenPos, radius * 1.5f * outerGlow, IM_COL32(cr, cg, cb, 60), 64, 1.5f);
    dl->AddCircle(screenPos, radius * 1.65f * outerGlow, IM_COL32(cr, cg, cb, 30), 64, 1.0f);
}

// ====================================================================================
// Animated BG for Menu
// ====================================================================================

static void DrawMenuBackground(ImDrawList* dl, ImVec2 pos, ImVec2 size, float time) {
    dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(18, 22, 36, 250), IM_COL32(24, 30, 48, 250),
        IM_COL32(16, 20, 34, 250), IM_COL32(22, 28, 44, 250));

    for (int i = 0; i < 50; i++) {
        float seed = (float)i * 12.9898f;
        float sx = fmodf(seed * 43758.5453f, 1.0f) * size.x;
        float sy = fmodf(seed * 23421.631f, 1.0f) * size.y;
        float drift = fmodf(time * 8.0f + i * 4.0f, size.y);
        sy = fmodf(sy + drift, size.y);
        float twinkle = 0.3f + 0.7f * (0.5f + 0.5f * sinf(time * 2.0f + i * 0.7f));
        int alpha = (int)(150 * twinkle);
        dl->AddCircleFilled(ImVec2(pos.x + sx, pos.y + sy), 1.0f + 1.5f * twinkle, IM_COL32(150, 200, 255, alpha));
    }

    for (int i = 0; i < 15; i++) {
        float y = fmodf(time * 40.0f + i * 30.0f, size.y);
        int alpha = (int)(25 + 15 * sinf(time * 2.0f + i));
        float startX = fmodf(time * 100.0f + i * 80.0f, size.x);
        dl->AddLine(ImVec2(pos.x + startX, pos.y + y), ImVec2(pos.x + startX + 100.0f, pos.y + y), IM_COL32(80, 200, 255, alpha), 1.5f);
    }
}

static void DrawRedBlueGlowEffect(ImDrawList* dl, ImVec2 center, float radius, float time) {
    int numParticles = 30;
    float baseAngle = time * 0.3f;

    for (int i = 0; i < numParticles; i++) {
        float angle = baseAngle + (i * 2.0f * PI / numParticles);
        float wave = sinf(time * 1.5f + i * 0.5f) * 0.15f + 0.85f;
        float r = (radius + 4.0f) * (1.0f + wave * 0.12f);
        float x = center.x + cosf(angle) * r;
        float y = center.y + sinf(angle) * r;
        float alpha = (0.4f + 0.6f * (0.5f + 0.5f * sinf(time * 2.5f + i * 0.7f)));
        int alphaInt = (int)(alpha * 200);
        float mix = 0.5f + 0.5f * sinf(time * 0.5f + i * 0.3f);
        ImU32 col = (mix > 0.5f) ? IM_COL32(60, 180, 255, alphaInt) : IM_COL32(255, 60, 60, alphaInt);
        float dotSize = 2.0f + 4.0f * (0.5f + 0.5f * sinf(time * 3.0f + i * 0.8f));
        dl->AddCircleFilled(ImVec2(x, y), dotSize, col);
    }

    for (int i = 0; i < 4; i++) {
        float pulse = 1.0f + 0.1f * sinf(time * 1.2f + i * 1.3f);
        float r = (radius + 6.0f + i * 5.0f) * pulse;
        int alpha = (int)(40 - i * 5 + 25 * sinf(time * 1.5f + i));
        float mix = 0.5f + 0.5f * sinf(time * 0.3f + i * 0.5f);
        ImU32 col = (mix > 0.5f) ? IM_COL32(60, 180, 255, alpha) : IM_COL32(255, 60, 60, alpha);
        dl->AddCircle(center, r, col, 0, 2.0f - i * 0.3f);
    }
}

#include "mod/ButtonClicker.h"

// ====================================================================================
// DRAW TOGGLE BUTTON
// ====================================================================================

static void DrawToggleButton(bool cancelMode, float screenScale) {
    if (g_menu.isOpen) return;

    ImGuiIO& io = GetIO();
    static GLuint play_on_tex = LoadTextureFromMemory(play_on_png, play_on_png_len);
    static GLuint play_off_tex = LoadTextureFromMemory(play_off_png, play_off_png_len);
    static GLuint queue_cancel_tex = LoadTextureFromMemory(play_on_png, play_on_png_len);

    float button_size = 72.f * screenScale;
    float windowWidth = button_size + GetStyle().WindowPadding.x * 2.0f * screenScale;
    float windowHeight = button_size + GetStyle().WindowPadding.y * 2.0f * screenScale;
    const float rightMargin = 20.0f * screenScale;

    float fixedX = io.DisplaySize.x - rightMargin - windowWidth;

    SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
    SetNextWindowPos(ImVec2(fixedX, g_sideBtnsY), ImGuiCond_Always);

    PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 99.0f * screenScale);

    if (Begin(O("##ToggleBtn"), nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {

        ImVec2 pos = GetCursorScreenPos();
        ImVec2 size(button_size, button_size);
        ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

        g_toggleRotAngle += io.DeltaTime * 1.5f;
        if (g_toggleRotAngle > 360.0f) g_toggleRotAngle -= 360.0f;

        if (InvisibleButton(O("##TglBtnHit"), size)) {
            AutoPlay::bAutoPlaying = !AutoPlay::bAutoPlaying;
            if (AutoPlay::bAutoPlaying) {
                AutoPlay::ClearState();
                AutoPlay::state = AutoPlay::SCANNING;
            }
            save_persistence();
        }

        GLuint tex;
        if (cancelMode) tex = queue_cancel_tex;
        else if (AutoPlay::bAutoPlaying) tex = play_on_tex;
        else tex = play_off_tex;

        float r = size.x * 0.5f;
        ImDrawList* dl = GetWindowDrawList();

        if (AutoPlay::bAutoPlaying && !cancelMode) {
            for (int i = 0; i < 3; i++) {
                float phase = fmodf(g_toggleRotAngle * 0.5f + i * 0.33f, 1.0f);
                float animR = r + phase * 22.0f * screenScale;
                int alpha = (int)(180 * (1.0f - phase) * (1.0f - phase));
                if (alpha < 0) alpha = 0;
                if (alpha > 255) alpha = 255;
                dl->AddCircle(center, animR, IM_COL32(255, 80, 80, alpha), 64, 2.5f * screenScale);
            }
            
            float goldPulse = 0.5f + 0.5f * sinf(g_toggleRotAngle * 1.2f);
            for (int i = 0; i < 3; i++) {
                float gr = r + 8.0f * screenScale + i * 3.0f * screenScale;
                int alpha = (int)(100 * goldPulse / (i + 1));
                if (alpha < 0) alpha = 0;
                if (alpha > 255) alpha = 255;
                dl->AddCircle(center, gr, IM_COL32(255, 210, 60, alpha), 64, 2.0f * screenScale);
            }
            
            for (int i = 0; i < 12; i++) {
                float angle = g_toggleRotAngle * 1.5f + (i * PI * 2.0f / 12.0f);
                float pr = r + 6.0f * screenScale;
                float px = center.x + cosf(angle) * pr;
                float py = center.y + sinf(angle) * pr;
                int alpha = (int)(200 + 55 * sinf(g_toggleRotAngle * 3.0f + i * 0.5f));
                if (alpha > 255) alpha = 255;
                if (alpha < 0) alpha = 0;
                float ptSize = 2.5f + 1.5f * sinf(g_toggleRotAngle * 2.0f + i);
                dl->AddCircleFilled(ImVec2(px, py), ptSize * screenScale, IM_COL32(255, 220, 80, alpha));
            }
            
            float offsetX = 3.0f * screenScale * sinf(g_toggleRotAngle * 0.4f);
            float offsetY = 3.0f * screenScale * cosf(g_toggleRotAngle * 0.4f);
            dl->AddImage((void*)(intptr_t)tex, 
                ImVec2(center.x - r + offsetX, center.y - r + offsetY), 
                ImVec2(center.x + r + offsetX, center.y + r + offsetY));
            
            int edgeAlpha = (int)(180 + 75 * sinf(g_toggleRotAngle * 1.8f));
            if (edgeAlpha > 255) edgeAlpha = 255;
            if (edgeAlpha < 0) edgeAlpha = 0;
            dl->AddCircle(center, r + 3.0f * screenScale, IM_COL32(255, 80, 80, edgeAlpha), 64, 3.0f * screenScale);
            dl->AddCircle(center, r + 5.0f * screenScale, IM_COL32(255, 210, 60, edgeAlpha / 2), 64, 1.5f * screenScale);
            
        } else if (cancelMode) {
            for (int i = 0; i < 3; i++) {
                float animR = r + (i + 1) * 4.0f * screenScale + 3.0f * sinf(g_toggleRotAngle * 2.0f + i);
                int alpha = (int)(130 - i * 30 + 50 * sinf(g_toggleRotAngle * 3.0f + i));
                if (alpha < 0) alpha = 0;
                if (alpha > 255) alpha = 255;
                dl->AddCircle(center, animR, IM_COL32(255, 200, 0, alpha), 64, 2.5f * screenScale);
            }
            for (int i = 0; i < 8; i++) {
                float angle = -g_toggleRotAngle * 2.0f + (i * PI * 2.0f / 8.0f);
                float pr = r + 5.0f * screenScale;
                float px = center.x + cosf(angle) * pr;
                float py = center.y + sinf(angle) * pr;
                int alpha = (int)(180 + 75 * sinf(g_toggleRotAngle * 3.5f + i));
                if (alpha > 255) alpha = 255;
                if (alpha < 0) alpha = 0;
                dl->AddCircleFilled(ImVec2(px, py), 2.5f * screenScale, IM_COL32(255, 220, 80, alpha));
            }
            dl->AddImage((void*)(intptr_t)tex, 
                ImVec2(center.x - r, center.y - r), 
                ImVec2(center.x + r, center.y + r));
            
        } else {
            dl->AddImage((void*)(intptr_t)tex, 
                ImVec2(center.x - r, center.y - r), 
                ImVec2(center.x + r, center.y + r));
        }
    }
    End();
    PopStyleVar();
    PopStyleColor(2);
}

// ====================================================================================
// DRAW ESP — الأصلي مع حل التجميد فقط
// ====================================================================================

INLINE void DrawESP(ImDrawList* draw) {
    if (!draw) return;
    if (!sharedGameManager) return;

    ImGuiIO& io = GetIO();
    float screenScale = io.DisplaySize.x / 1080.0f;
    if (screenScale < 0.6f) screenScale = 0.6f;
    if (screenScale > 1.3f) screenScale = 1.3f;

    if (persistent_int["iESPCircleSize"] < 5) persistent_int["iESPCircleSize"] = 16;
    if (persistent_int["iESPCircleSize"] > 40) persistent_int["iESPCircleSize"] = 40;

    g_pocketGlowTime += io.DeltaTime;

    {
        if (!sharedGameManager) return;

        // ✅ 1. تحقق أولاً (بدون UpdateScreenTable بعد)
        sharedDirector = F(ptr, libmain + O(0x4f06288));
        if (!sharedDirector) return;
        
        sharedUserInfo = F(ptr, libmain + O(0x4e9feb8));
        if (!sharedUserInfo) return;

        // ✅ 2. Patch مرة وحدة فقط (بدل كل كادر)
        if (!g_userInfoPatched) {
            F(bool, sharedUserInfo + 0x340) = true;
            g_userInfoPatched = true;
        }

        sharedMainManager = F(ptr, libmain + O(0x4dde3e0));
        if (!sharedMainManager) return;
        
        sharedMenuManager = F(ptr, libmain + O(0x4dfe838));
        if (!sharedMenuManager) return;

        MainStateManager mainStateManager = sharedMainManager.mStateManager;
        if (!mainStateManager) return;

        // ✅ 3. isInGame cached — كل 100ms بدل كل كادر
        float nowTime = ImGui::GetTime();
        if (nowTime - g_lastInGameCheck > 0.1f) {
            g_isInGameCached = mainStateManager.isInGame();
            g_lastInGameCheck = nowTime;
        }
        
        if (!g_isInGameCached) return;

        // ✅ 4. الآن داخل لعبة فعلاً — UpdateScreenTable
        UpdateScreenTable();

        auto visualCue = sharedGameManager.mVisualCue();
        Ball::Classification myclass = sharedGameManager.getPlayerClassification();
        Table table = sharedGameManager.mTable;
        if (!table) return;
        auto tableProperties = table.mTableProperties();
        if (!tableProperties) return;
        auto& pockets = tableProperties.mPockets();

        GameStateManager gameStateManager = sharedGameManager.mStateManager;
        if (!gameStateManager) return;
        auto stateId = gameStateManager.getCurrentStateId();

        DrawToggleButton(false, screenScale);

        // ✅ AutoPlay: throttle كل 100ms
        if (persistent_bool[O("bAutoPlay")] || AutoPlay::bAutoPlaying) {
            float now = ImGui::GetTime();
            if (now - g_lastAutoPlayUpdate > 0.1f) {
                AutoPlay::Update();
                g_lastAutoPlayUpdate = now;
            }
            
            if (false) {
//                AutoPlay::MoveAimSmooth(AutoPlay_GetSmoothAimAngle(), 0.025);
            }
        }
        
        if (stateId != 6 && stateId != 7 && stateId != 8 && stateId != 9) {
            gPrediction->determineShotResult(false);
        }
        if (stateId == 6 || stateId == 7 || stateId == 8) return;

        if (persistent_bool[O("bESP_DrawPocketsShotState")]) {
            for (int i = 0; i < 6; i++) {
                if (Prediction::pocketStatus[i]) {
                    auto screenPos = WorldToScreen(pockets[i]);
                    DrawPocketGlowEffect(draw, ImVec2(screenPos.x, screenPos.y), 30.0f * screenScale, g_pocketGlowTime, IM_COL32(255, 60, 60, 255));
                    draw->AddCircle(ImVec2(screenPos.x, screenPos.y), 30 * screenScale, GREEN, 0, 5.f * screenScale);
                }
            }
        }

        if (persistent_bool[O("bESP_DrawPredictionLine")]) {
            for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
                auto& ball = gPrediction->guiData.balls[i];
                if (ball.initialPosition != ball.predictedPosition) {
                    ImVec2 lastPos{};
                    float lineThick = (float)persistent_int[O("iLineThickness")] * screenScale;
                    if (lineThick < 1.f * screenScale) lineThick = 1.f * screenScale;
                    bool isStripe = (i >= 9 && i <= 15);

                    for (int j = 1; j < (int)ball.positions.size(); j++) {
                        auto point = WorldToScreen(ball.positions[j]);
                        if (lastPos.x || lastPos.y) {
                            if (isStripe) {
                                float dx = point.x - lastPos.x;
                                float dy = point.y - lastPos.y;
                                float segLen = sqrtf(dx * dx + dy * dy);
                                if (segLen < 0.0001f) { lastPos = point; continue; }
                                float dashLen = lineThick * 3.0f + 6.0f * screenScale;
                                float gapLen = lineThick * 3.0f + 6.0f * screenScale;
                                float stepLen = dashLen + gapLen;
                                int steps = (int)(segLen / stepLen);
                                if (steps < 1) steps = 1;
                                float ux = dx / segLen;
                                float uy = dy / segLen;
                                ImU32 colBall = colors[i];
                                for (int k = 0; k <= steps; k++) {
                                    float t0 = (float)k * stepLen;
                                    float t1 = t0 + dashLen;
                                    if (t0 >= segLen) break;
                                    if (t1 > segLen) t1 = segLen;
                                    ImVec2 a(lastPos.x + ux * t0, lastPos.y + uy * t0);
                                    ImVec2 b(lastPos.x + ux * t1, lastPos.y + uy * t1);
                                    draw->AddLine(a, b, colBall, lineThick);
                                }
                            } else {
                                draw->AddLine(lastPos, point, colors[i], lineThick);
                            }
                        }
                        lastPos = point;
                    }
                }
            }
        }

        if (persistent_bool[O("bESP_DrawPredictionLine")]) {
            for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
                auto& ball = gPrediction->guiData.balls[i];
                if (ball.initialPosition != ball.predictedPosition) {
                    float circleR = ((float)persistent_int[O("iLineThickness")] + 1.f) * screenScale;
                    if (circleR < 2.f * screenScale) circleR = 2.f * screenScale;
                    draw->AddCircleFilled(WorldToScreen(ball.initialPosition), circleR, colors[i]);
                    float circleSize = (float)persistent_int["iESPCircleSize"] * screenScale;
                    if (circleSize < 5 * screenScale) circleSize = 5 * screenScale;
                    if (circleSize > 40 * screenScale) circleSize = 40 * screenScale;
                    draw->AddCircleFilled(WorldToScreen(ball.predictedPosition), circleSize, colors[i]);
                }
            }
        }
    }
}

// ====================================================================================
// TOGGLE SWITCH
// ====================================================================================

static bool ToggleSwitch(const char* label, bool* v, float screenScale) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    float scale = 1.4f * screenScale;
    float height = 28.0f * scale;
    float width = 50.0f * scale;
    float radius = height * 0.5f;

    ImVec2 textSize = CalcTextSize(label);
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(GetContentRegionAvail().x, ImMax(height, textSize.y) + style.FramePadding.y * 2.0f + 8.0f * screenScale);
    const ImRect bb(pos, pos + size);

    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered = false, held = false;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !(*v);

    static std::map<ImGuiID, float> switchAnim;
    float& animT = switchAnim[id];
    float targetT = *v ? 1.0f : 0.0f;
    animT += (targetT - animT) * g.IO.DeltaTime * 14.0f;

    ImDrawList* dl = window->DrawList;

    if (hovered) {
        dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(60, 140, 255, 30), 8.0f * screenScale);
    }

    ImVec2 togglePos = ImVec2(bb.Max.x - width - 12.0f * screenScale, bb.Min.y + (size.y - height) * 0.5f);
    ImVec2 toggleEnd = ImVec2(togglePos.x + width, togglePos.y + height);

    ImVec4 offColor = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    ImVec4 onColor = ImVec4(0.90f, 0.15f, 0.15f, 1.0f);
    ImVec4 bgColorV = ImLerp(offColor, onColor, animT);

    dl->AddRectFilled(togglePos, toggleEnd, ImColor(bgColorV), radius);
    dl->AddRect(togglePos, toggleEnd, IM_COL32(80, 100, 140, 200), radius, 0, 1.0f * screenScale);

    if (animT > 0.1f) {
        dl->AddRect(togglePos, toggleEnd, IM_COL32(255, 60, 60, (int)(animT * 180.0f)), radius, 0, 1.2f * screenScale);
    }

    float knobX = togglePos.x + radius + (width - height) * animT;
    float knobY = togglePos.y + radius;
    float knobR = radius - 3.0f * screenScale;

    dl->AddCircleFilled(ImVec2(knobX + 1.5f * screenScale, knobY + 1.5f * screenScale), knobR + 2.0f * screenScale, IM_COL32(0, 0, 0, 80));
    dl->AddCircleFilled(ImVec2(knobX, knobY), knobR, IM_COL32(255, 255, 255, 255));

    if (animT > 0.5f) {
        dl->AddCircle(ImVec2(knobX, knobY), knobR, IM_COL32(255, 60, 60, 255), 0, 1.5f * screenScale);
    }

    dl->AddText(ImVec2(bb.Min.x + 12.0f * screenScale, bb.Min.y + (size.y - textSize.y) * 0.5f), WHITE_TEXT, label);

    return pressed;
}

// ====================================================================================
// DRAW UNIFIED MENU
// ====================================================================================

static void DrawUnifiedMenu(float winW, float winH, float screenScale) {
    bool need_save = false;

    ImDrawList* dl = GetWindowDrawList();
    ImVec2 wp = GetWindowPos();

    float contentW = winW;

    g_menu.waveTime += ImGui::GetIO().DeltaTime;

    DrawMenuBackground(dl, wp, ImVec2(contentW, winH), g_menu.waveTime);

    float headerH = 45.0f * screenScale;
    float padding = 10.0f * screenScale;
    float btnSize = 28.0f * screenScale;

    dl->AddRectFilledMultiColor(
        wp,
        ImVec2(wp.x + contentW, wp.y + headerH),
        IM_COL32(28, 34, 54, 250),
        IM_COL32(22, 28, 46, 250),
        IM_COL32(26, 32, 50, 250),
        IM_COL32(30, 36, 58, 250)
    );

    for (int i = 0; i < (int)contentW; i += 2) {
        float p = (float)i / contentW;
        float hue = fmodf(p + g_menu.waveTime * 0.2f, 1.0f);
        ImU32 c;
        if (hue < 0.33f) c = IM_COL32(120, 90, 200, 255);
        else if (hue < 0.66f) c = IM_COL32(80, 180, 255, 255);
        else c = IM_COL32(255, 100, 200, 255);
        dl->AddLine(ImVec2(wp.x + i, wp.y), ImVec2(wp.x + i + 2, wp.y), c, 2.0f * screenScale);
    }

    const char* titleLeft = "[  SHEFO 56.28.5 V1 VIP ]";
    const char* titleRight = "- [ @SHEFO_0 ]";

    SetWindowFontScale(1.15f);
    ImVec2 titleLeftSize = CalcTextSize(titleLeft);
    ImVec2 titleRightSize = CalcTextSize(titleRight);
    float totalTitleW = titleLeftSize.x + titleRightSize.x + 8.0f * screenScale;
    float titleStartX = (contentW - totalTitleW) * 0.5f;
    ImVec2 titlePos = ImVec2(wp.x + titleStartX, wp.y + (headerH - titleLeftSize.y) * 0.5f);

    dl->AddText(titlePos, IM_COL32(230, 235, 255, 255), titleLeft);
    dl->AddText(ImVec2(titlePos.x + titleLeftSize.x + 8.0f * screenScale, titlePos.y), IM_COL32(255, 200, 80, 255), titleRight);

    SetWindowFontScale(1.0f);

    float closeX = contentW - btnSize - padding;
    float closeY = (headerH - btnSize) * 0.5f;
    ImGuiID closeId = GetID("##CloseBtn");
    ImRect closeBb(ImVec2(wp.x + closeX, wp.y + closeY), ImVec2(wp.x + closeX + btnSize, wp.y + closeY + btnSize));
    ItemSize(closeBb.GetSize());
    if (ItemAdd(closeBb, closeId)) {
        bool hovered, held;
        bool pressed = ButtonBehavior(closeBb, closeId, &hovered, &held);
        if (pressed) g_menu.isOpen = false;

        float cx = closeBb.Min.x + btnSize * 0.5f;
        float cy = closeBb.Min.y + btnSize * 0.5f;

        ImU32 bgCol = hovered ? IM_COL32(255, 80, 80, 255) : IM_COL32(200, 40, 40, 220);
        dl->AddCircleFilled(ImVec2(cx, cy), btnSize * 0.45f, bgCol);
        dl->AddCircle(ImVec2(cx, cy), btnSize * 0.45f, IM_COL32(255, 120, 120, 255), 0, 1.5f);

        float l = btnSize * 0.18f;
        ImU32 xCol = IM_COL32(255, 255, 255, 255);
        dl->AddLine(ImVec2(cx - l, cy - l), ImVec2(cx + l, cy + l), xCol, 2.5f);
        dl->AddLine(ImVec2(cx + l, cy - l), ImVec2(cx - l, cy + l), xCol, 2.5f);
    }

    float lineY = headerH;
    float gradProgress = fmodf(g_menu.waveTime * 0.3f, 1.0f);
    for (int i = 0; i < (int)contentW; i += 2) {
        float p = (float)i / contentW;
        float hue = fmodf(p + gradProgress, 1.0f);
        ImU32 color;
        if (hue < 0.33f) color = IM_COL32(255, 100, 100, 200);
        else if (hue < 0.66f) color = IM_COL32(100, 200, 255, 200);
        else color = IM_COL32(180, 130, 255, 200);
        dl->AddLine(ImVec2(wp.x + i, wp.y + lineY), ImVec2(wp.x + i + 2, wp.y + lineY), color, 1.5f * screenScale);
    }

    float contentStartY = headerH + 8.0f * screenScale;
    float contentH = winH - contentStartY - 8.0f * screenScale;
    float colGap = 8.0f * screenScale;
    float colW = (contentW - padding * 3.0f - colGap) / 2.0f;
    float leftX = padding;
    float rightX = padding * 2.0f + colW;

    dl->AddRectFilled(ImVec2(wp.x + leftX, wp.y + contentStartY), ImVec2(wp.x + leftX + colW, wp.y + contentStartY + contentH), IM_COL32(24, 30, 48, 235), 10.0f * screenScale);
    dl->AddRect(ImVec2(wp.x + leftX, wp.y + contentStartY), ImVec2(wp.x + leftX + colW, wp.y + contentStartY + contentH), IM_COL32(80, 100, 160, 180), 10.0f * screenScale, 0, 1.2f * screenScale);

    dl->AddRectFilled(ImVec2(wp.x + rightX, wp.y + contentStartY), ImVec2(wp.x + rightX + colW, wp.y + contentStartY + contentH), IM_COL32(24, 30, 48, 235), 10.0f * screenScale);
    dl->AddRect(ImVec2(wp.x + rightX, wp.y + contentStartY), ImVec2(wp.x + rightX + colW, wp.y + contentStartY + contentH), IM_COL32(80, 100, 160, 180), 10.0f * screenScale, 0, 1.2f * screenScale);

    SetCursorPos(ImVec2(leftX + 12.0f * screenScale, contentStartY + 10.0f * screenScale));
    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    BeginChild("##LeftPanel", ImVec2(colW - 24.0f * screenScale, contentH - 20.0f * screenScale), true, ImGuiWindowFlags_NoBackground);

    TextColored(PURPLE_VEC4, "[ PREMIUM CONTROLS ]");
    Dummy(ImVec2(0, 4 * screenScale));

    need_save |= ToggleSwitch("Lines", &persistent_bool[O("bESP_DrawPredictionLine")], screenScale);
    need_save |= ToggleSwitch("Pockets", &persistent_bool[O("bESP_DrawPocketsShotState")], screenScale);
    
    {
        bool prev = persistent_bool[O("bAutoPlay")];
        bool toggled = ToggleSwitch("AutoPlay", &persistent_bool[O("bAutoPlay")], screenScale);
        if (toggled || persistent_bool[O("bAutoPlay")] != prev) {
            AutoPlay::bAutoPlaying = persistent_bool[O("bAutoPlay")];
            if (AutoPlay::bAutoPlaying) {
                AutoPlay::ClearState();
                AutoPlay::state = AutoPlay::SCANNING;
            } else {
                AutoPlay::ClearState();
            }
            need_save = true;
        }
    }

    Dummy(ImVec2(0, 6 * screenScale));

    TextColored(CYAN_VEC4, "[ LINE THICKNESS ]");
    Dummy(ImVec2(0, 3 * screenScale));
    {
        if (persistent_int[O("iLineThickness")] < 1) persistent_int[O("iLineThickness")] = 4;
        PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * screenScale);
        PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f * screenScale);
        PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.18f, 0.28f, 1.0f));
        PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.24f, 0.35f, 1.0f));
        PushStyleColor(ImGuiCol_SliderGrab, SLIDER_BLUE_VEC4);
        PushStyleColor(ImGuiCol_SliderGrabActive, SLIDER_BLUE_ACTIVE_VEC4);
        SetNextItemWidth(GetContentRegionAvail().x);
        need_save |= SliderInt("##lineThick", &persistent_int[O("iLineThickness")], 1, 10, "SIZE: %d");
        PopStyleColor(4);
        PopStyleVar(2);
    }

    Dummy(ImVec2(0, 6 * screenScale));

    TextColored(CYAN_VEC4, "[ ESP CIRCLE SIZE ]");
    Dummy(ImVec2(0, 3 * screenScale));
    {
        if (persistent_int["iESPCircleSize"] < 5) persistent_int["iESPCircleSize"] = 16;
        if (persistent_int["iESPCircleSize"] > 40) persistent_int["iESPCircleSize"] = 40;
        PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * screenScale);
        PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f * screenScale);
        PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.18f, 0.28f, 1.0f));
        PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.24f, 0.35f, 1.0f));
        PushStyleColor(ImGuiCol_SliderGrab, SLIDER_BLUE_VEC4);
        PushStyleColor(ImGuiCol_SliderGrabActive, SLIDER_BLUE_ACTIVE_VEC4);
        SetNextItemWidth(GetContentRegionAvail().x);
        need_save |= SliderInt("##circleSize", &persistent_int["iESPCircleSize"], 5, 40, "SIZE: %d");
        PopStyleColor(4);
        PopStyleVar(2);
    }

    Dummy(ImVec2(0, 6 * screenScale));

    TextColored(ORANGE_VEC4, "[ SHOT MODE ]");
    Dummy(ImVec2(0, 3 * screenScale));
    {
        int& mode = persistent_int["iShotMode"];
        if (mode < 0 || mode > 2) mode = 1;
        
        float avail = GetContentRegionAvail().x;
        float gap = 5.0f * screenScale;
        float bW = (avail - gap * 2.0f) / 3.0f;
        float bH = 35.0f * screenScale;
        PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * screenScale);
        
        const char* labels[3] = { "SINGLE", "BALANCED", "AGGR" };
        
        for (int i = 0; i < 3; i++) {
            if (i > 0) SameLine(0, gap);
            bool isSel = (mode == i);
            
            if (isSel) {
                if (i == 0) {
                    PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.95f, 1.0f));
                    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 1.0f, 1.0f));
                    PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.45f, 0.85f, 1.0f));
                } else if (i == 1) {
                    PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.70f, 0.35f, 1.0f));
                    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.80f, 0.45f, 1.0f));
                    PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.60f, 0.25f, 1.0f));
                } else {
                    PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.35f, 0.10f, 1.0f));
                    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.45f, 0.20f, 1.0f));
                    PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.25f, 0.05f, 1.0f));
                }
                PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            } else {
                PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.18f, 0.28f, 1.0f));
                PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.24f, 0.35f, 1.0f));
                PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.13f, 0.22f, 1.0f));
                PushStyleColor(ImGuiCol_Text, ImVec4(0.63f, 0.70f, 0.82f, 1.0f));
            }
            
            if (Button(labels[i], ImVec2(bW, bH))) {
                mode = i;
                need_save = true;
            }
            PopStyleColor(4);
        }
        PopStyleVar();
    }

    Dummy(ImVec2(0, 6 * screenScale));

    TextColored(PINK_VEC4, "[ MENU SIZE ]");
    Dummy(ImVec2(0, 3 * screenScale));
    {
        PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * screenScale);
        PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f * screenScale);
        PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.18f, 0.28f, 1.0f));
        PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.24f, 0.35f, 1.0f));
        PushStyleColor(ImGuiCol_SliderGrab, SLIDER_PINK_VEC4);
        PushStyleColor(ImGuiCol_SliderGrabActive, SLIDER_PINK_ACTIVE_VEC4);
        SetNextItemWidth(GetContentRegionAvail().x);
        int& menuSz = persistent_int[O("iMenuSizeOffset")];
        bool changed = SliderInt("##menuSize", &menuSz, -10, 10, menuSz == 0 ? "Normal" : "%d");
        need_save |= changed;
        PopStyleColor(4);
        PopStyleVar(2);
    }

    Dummy(ImVec2(0, 12 * screenScale));

    TextColored(GOLD_VEC4, "[ SAVE SETTINGS ]");
    Dummy(ImVec2(0, 4 * screenScale));
    
    PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.3f, 1.0f));
    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.75f, 0.4f, 1.0f));
    PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.5f, 0.25f, 1.0f));
    PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f * screenScale);
    
    float saveBtnW = GetContentRegionAvail().x;
    if (Button("SAVE SETTINGS", ImVec2(saveBtnW, 45.0f * screenScale))) {
        save_persistence();
        need_save = false;
        g_saveNotifTimer = 2.5f;
        LOGI("Settings saved manually!");
    }
    
    PopStyleVar();
    PopStyleColor(4);

    EndChild();
    PopStyleColor();

    SetCursorPos(ImVec2(rightX + 12.0f * screenScale, contentStartY + 10.0f * screenScale));
    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    BeginChild("##RightPanel", ImVec2(colW - 24.0f * screenScale, contentH - 20.0f * screenScale), true, ImGuiWindowFlags_NoBackground);

    TextColored(CYAN_VEC4, "[ SYSTEM STATUS ]");
    Dummy(ImVec2(0, 4 * screenScale));

    TextColored(GREY_BLUE_VEC4, "Server: ");
    SameLine();
    {
        float pulse = 0.5f + 0.5f * sinf(g_menu.waveTime * 4.0f);
        ImGui::TextColored(ImVec4(0.0f, 0.6f + 0.4f * pulse, 0.0f, 1.0f), "ONLINE");
    }

    Dummy(ImVec2(0, 3 * screenScale));

    {
        ImVec2 p = GetCursorScreenPos();
        float w = GetContentRegionAvail().x;
        float h = 24.0f * screenScale;
        dl->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + h), IM_COL32(20, 40, 30, 220), 6.0f * screenScale);
        dl->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + h), IM_COL32(0, 200, 100, 200), 6.0f * screenScale, 0, 1.0f);
        dl->AddText(ImVec2(p.x + 8.0f * screenScale, p.y + 4.0f * screenScale), IM_COL32(0, 255, 120, 255), "• ACTIVE");
        Dummy(ImVec2(w, h));
    }

    Dummy(ImVec2(0, 4 * screenScale));

    static char s_manufacturer[PROP_VALUE_MAX] = {};
    static char s_model[PROP_VALUE_MAX] = {};
    static char s_abi[PROP_VALUE_MAX] = {};
    static char s_android[PROP_VALUE_MAX] = {};
    static bool s_props_loaded = false;

    if (!s_props_loaded) {
        __system_property_get("ro.product.manufacturer", s_manufacturer);
        __system_property_get("ro.product.model", s_model);
        __system_property_get("ro.product.cpu.abi", s_abi);
        __system_property_get("ro.build.version.release", s_android);
        s_props_loaded = true;
    }

    TextColored(GREY_BLUE_VEC4, "Brand: ");
    SameLine();
    TextColored(CYAN_VEC4, "%s", s_manufacturer);

    TextColored(GREY_BLUE_VEC4, "Device: ");
    SameLine();
    TextColored(CYAN_VEC4, "%s", s_model);

    TextColored(GREY_BLUE_VEC4, "Android: ");
    SameLine();
    TextColored(CYAN_VEC4, "%s", s_android);

    TextColored(GREY_BLUE_VEC4, "Arch: ");
    SameLine();
    TextColored(CYAN_VEC4, "%s", s_abi);

    TextColored(GREY_BLUE_VEC4, "Version: ");
    SameLine();
    TextColored(ORANGE_VEC4, "V1 / 56.28.5");

    TextColored(GREY_BLUE_VEC4, "LICENSE: ");
    SameLine();
    TextColored(GREEN_VEC4, "Lifetime / No Login");

    Dummy(ImVec2(0, 6 * screenScale));

    {
        ImVec2 p = GetCursorScreenPos();
        float w = GetContentRegionAvail().x;
        dl->AddLine(p, ImVec2(p.x + w, p.y), IM_COL32(100, 130, 200, 150), 1.0f);
        Dummy(ImVec2(w, 2.0f * screenScale));
    }

    Dummy(ImVec2(0, 4 * screenScale));

    TextColored(RED_VEC4, "• TG: ");
    SameLine();
    TextColored(CYAN_VEC4, "@SHEFO_0");

    Dummy(ImVec2(0, 6 * screenScale));

    TextColored(CYAN_VEC4, "[ STATUS ]");
    Dummy(ImVec2(0, 8 * screenScale));
    {
        ImVec2 p = GetCursorScreenPos();
        float availW = GetContentRegionAvail().x;
        float boxH = 48.0f * screenScale;
        ImVec2 boxMin(p.x, p.y);
        ImVec2 boxMax(p.x + availW, p.y + boxH);
        dl->AddRectFilled(boxMin, boxMax, IM_COL32(24, 60, 40, 240), 7.0f * screenScale);
        dl->AddRect(boxMin, boxMax, IM_COL32(80, 200, 130, 200), 7.0f * screenScale, 0, 1.2f);
        SetWindowFontScale(1.05f);
        ImVec2 valueSize = CalcTextSize("ACTIVE");
        dl->AddText(ImVec2(p.x + (availW - valueSize.x) * 0.5f, p.y + 3.0f * screenScale), IM_COL32(210, 255, 225, 255), "ACTIVE");
        SetWindowFontScale(1.0f);
        Dummy(ImVec2(availW, boxH));
    }
    Dummy(ImVec2(0, 4 * screenScale));
    EndChild();
    PopStyleColor();

    if (g_saveNotifTimer > 0.0f) {
        g_saveNotifTimer -= ImGui::GetIO().DeltaTime;
        
        float alpha = g_saveNotifTimer > 0.5f ? 1.0f : g_saveNotifTimer / 0.5f;
        if (alpha < 0.0f) alpha = 0.0f;
        
        SetNextWindowPos(ImVec2(wp.x + winW * 0.5f, wp.y + winH - 80.0f * screenScale), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.5f, 0.25f, alpha * 0.95f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.8f, 0.4f, alpha));
        PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * screenScale);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f * screenScale, 12.0f * screenScale));
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        
        if (Begin("##SaveNotif", nullptr, 
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                  ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing)) {
            
            SetWindowFontScale(1.3f);
            TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "✓  SETTINGS SAVED");
            SetWindowFontScale(1.0f);
        }
        End();
        
        PopStyleVar(4);
        PopStyleColor(2);
    }

    // ✅ save_persistence: مرة كل ثانية بدل كل كادر
    if (need_save) {
        float now = ImGui::GetTime();
        if (now - g_lastSaveTime > 1.0f) {
            save_persistence();
            need_save = false;
            g_lastSaveTime = now;
        }
    }
}

// ====================================================================================
// DRAW MENU
// ====================================================================================

INLINE void DrawMenu(ImGuiIO& io) {
    {
        if (is_segv_handler_active()) {
            jump_buffer_active = 1;
            if (!sigsetjmp(jump_buffer, 1)) DrawESP(GetBackgroundDrawList());
            jump_buffer_active = 0;
        }

        if (g_menu.isOpen) {
            g_menu.menuAlpha += (1.0f - g_menu.menuAlpha) * io.DeltaTime * 12.0f;
            g_menu.slideOffset += (0.0f - g_menu.slideOffset) * io.DeltaTime * 8.0f;
            if (g_menu.menuAlpha > 0.99f) g_menu.menuAlpha = 1.0f;
            if (fabsf(g_menu.slideOffset) < 0.001f) g_menu.slideOffset = 0.0f;
        } else {
            g_menu.menuAlpha = 0.0f;
            g_menu.slideOffset = -30.0f;
        }

        if (g_menu.menuAlpha > 0.01f) {
            float screenScale = io.DisplaySize.x / 1080.0f;
            if (screenScale < 0.6f) screenScale = 0.6f;
            if (screenScale > 1.3f) screenScale = 1.3f;

            float sizeScale = 1.0f + (float)persistent_int[O("iMenuSizeOffset")] * 0.03f;
            if (sizeScale < 0.3f) sizeScale = 0.3f;

            float winW = 950.0f * sizeScale * screenScale;
            float winH = 630.0f * sizeScale * screenScale;

            ImVec2 pos = ImVec2(Width / 2.0f + g_menu.slideOffset, Height / 2.0f);

            SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
            SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f * screenScale);
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            PushStyleVar(ImGuiStyleVar_Alpha, g_menu.menuAlpha);

            ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoResize;

            if (Begin("##MainMenu", &g_menu.isOpen, winFlags)) {
                DrawUnifiedMenu(winW, winH, screenScale);
            }
            End();

            PopStyleVar(4);
            PopStyleColor();
        }
    }
}

// ====================================================================================
// DRAW FLOATING BUTTON
// ====================================================================================

static void DrawFloatingButton(ImGuiIO& io) {
    if (g_menu.isOpen) return;

    static GLuint logo_tex = LoadTextureFromMemory(logo_png, logo_png_len);
    float screenScale = io.DisplaySize.x / 1080.0f;
    if (screenScale < 0.6f) screenScale = 0.6f;
    if (screenScale > 1.3f) screenScale = 1.3f;

    g_menu.glowTime += io.DeltaTime;
    g_menu.pulseTime += io.DeltaTime;

    float buttonRadius = 40.0f * GetPulseScale(g_menu.pulseTime) * screenScale;
    float winSize = (buttonRadius * 2.0f) + 8.0f * screenScale;
    const float rightMargin = 20.0f * screenScale;

    if (g_sideBtnsY <= 0.0f) g_sideBtnsY = io.DisplaySize.y - 130.0f * screenScale;

    float toggleWidth = 100.f * screenScale + (GetStyle().WindowPadding.x * 2.0f * screenScale);
    float fixedX = io.DisplaySize.x - rightMargin - toggleWidth + (toggleWidth - winSize) * 0.5f;
    float posY = g_sideBtnsY - 110.0f * screenScale;

    SetNextWindowPos(ImVec2(fixedX, posY), ImGuiCond_Always);
    SetNextWindowSize(ImVec2(winSize, winSize), ImGuiCond_Always);

    PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (Begin("##FloatBtn", nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {

        ImDrawList* dl = GetWindowDrawList();
        ImVec2 center = ImVec2(fixedX + (winSize * 0.5f), posY + (winSize * 0.5f));

        DrawRedBlueGlowEffect(dl, center, buttonRadius, g_menu.glowTime);
        float mix = 0.5f + 0.5f * sinf(g_menu.pulseTime * 0.5f);
        ImU32 color = mix > 0.5f ? BLUE_BRIGHT : RED_BRIGHT;
        dl->AddCircle(center, buttonRadius + 2.0f * screenScale, color, 0, 2.0f * screenScale);

        for (int i = 0; i < 2; i++) {
            float ringPhase = fmodf(g_menu.glowTime * 0.8f + i * 0.5f, 1.0f);
            float ringR = buttonRadius + ringPhase * 25.0f * screenScale;
            int ringAlpha = (int)(150 * (1.0f - ringPhase));
            if (ringAlpha < 0) ringAlpha = 0;
            dl->AddCircle(center, ringR, IM_COL32(60, 180, 255, ringAlpha), 0, 2.0f * screenScale);
        }

        InvisibleButton("##FloatBtnHit", ImVec2(winSize, winSize));

        if (IsItemActive() && IsMouseDragging(ImGuiMouseButton_Left)) {
            g_sideBtnsY += io.MouseDelta.y;
            g_sideBtnsY = ImClamp(g_sideBtnsY, 140.0f * screenScale, io.DisplaySize.y - 130.0f * screenScale);
        }

        if (IsItemHovered() && IsMouseReleased(0) && ImGui::GetMouseDragDelta(0).y == 0.0f) {
            g_menu.isOpen = true;
        }

        if (logo_tex) {
            dl->AddImage((void*)(intptr_t)logo_tex, ImVec2(center.x - buttonRadius, center.y - buttonRadius), ImVec2(center.x + buttonRadius, center.y + buttonRadius));
        }
    }
    End();
    PopStyleVar(2);
    PopStyleColor();
}

// ====================================================================================
// SETUP IMGUI
// ====================================================================================

INLINE void SetupImgui() {
    PACKAGE_NAME = string(getcmdline());

    ImGui::CreateContext();

    auto& style = ImGui::GetStyle();
    auto& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.12f, 0.47f, 0.86f, 0.5f);
    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.18f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.24f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.30f, 0.42f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = SLIDER_BLUE_VEC4;
    style.Colors[ImGuiCol_SliderGrabActive] = SLIDER_BLUE_ACTIVE_VEC4;
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.18f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = RED_VEC4;
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.18f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = RED_VEC4;
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = RED_VEC4;

    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    switch_theme(current_theme);
    load_persistence();
    load_imgui_style();

    persistent_bool[O("bAutoQueue")] = false;

    if (persistent_int["iShotMode"] < 0 || persistent_int["iShotMode"] > 2) {
        persistent_int["iShotMode"] = 1;
    }

    static string INI_PATH = O("/data/user_de/0/") + PACKAGE_NAME + O("/no_backup/.ini");
    io.IniFilename = persistent_bool["bImguiAutoSave"] ? INI_PATH.c_str() : nullptr;
    io.ConfigWindowsMoveFromTitleBarOnly = persistent_bool["bMoveOnlyWithTitleBar"];

    ImFontConfig font_cfg;
    font_cfg.SizePixels = persistent_float["fFontScale"];
    io.Fonts->AddFontDefault(&font_cfg);

    {
        const char* arFonts[] = {
            "/system/fonts/NotoNaskhArabic-Regular.ttf",
            "/system/fonts/NotoNaskhArabicUI-Regular.ttf",
            "/system/fonts/NotoSansArabic-Regular.ttf",
            "/system/fonts/DroidNaskh-Regular.ttf",
            "/system/fonts/DroidSansArabic.ttf",
            "/system/fonts/Roboto-Regular.ttf",
        };
        ImFontConfig arCfg;
        arCfg.MergeMode = true;
        arCfg.PixelSnapH = true;
        arCfg.SizePixels = persistent_float["fFontScale"];
        const ImWchar* arRanges = nullptr;
        for (int i = 0; i < (int)(sizeof(arFonts) / sizeof(arFonts[0])); i++) {
            FILE* tf = fopen(arFonts[i], "rb");
            if (!tf) continue;
            fclose(tf);
            io.Fonts->AddFontFromFileTTF(arFonts[i], persistent_float["fFontScale"], &arCfg, arRanges);
            break;
        }
        io.Fonts->Build();
    }

    ImGui_ImplAndroid_Init();
    ImGui_ImplOpenGL3_Init("#version 300 es");

    bImguiSetup = true;
}

// ====================================================================================
// DEFINES(Draw)
// ====================================================================================

DEFINES(EGLBoolean, Draw, EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &Width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &Height);

    if (Width <= 0 || Height <= 0) return _Draw(dpy, surface);

    screenCenter = Vector2(Width / 2, Height / 2);

    if (!bImguiSetup) SetupImgui();

    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(Width, Height);
    ImGui::NewFrame();

    if (!is_segv_handler_active()) setup_global_segv_handler();

    {
        bool hideAll = persistent_bool[O("bHideAllCapture")];
        if (!hideAll) {
            DrawFloatingButton(io);
            DrawMenu(io);

            {
                SetNextWindowPos(ImVec2(Width * 0.5f, Height - 50.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
                PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
                PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                Begin("##PoweredBy", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoInputs);
                SetWindowFontScale(0.65f);
                TextColored(RED_VEC4, "TG: @SHEFO_0");
                SetWindowFontScale(1.0f);
                End();
                PopStyleVar(2);
                PopStyleColor();
            }
        } else {
            SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            SetNextWindowSize(ImVec2(120.0f, 120.0f), ImGuiCond_Always);
            PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (Begin("##CaptureRecover", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse)) {
                if (InvisibleButton("##CaptureRecoverBtn", ImVec2(120.0f, 120.0f))) {
                    persistent_bool[O("bHideAllCapture")] = false;
                }
            }
            End();
            PopStyleVar(2);
            PopStyleColor();
        }
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return _Draw(dpy, surface);
}

void __IMGUI__() {
    create_directory_recursive(CONC("/data/user_de/0/", PACKAGE_NAME.c_str(), "/no_backup/"));
}