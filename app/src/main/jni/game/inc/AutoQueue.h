#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <algorithm>

using namespace std;

// ═══════════════════════════════════════════════════════════════
// ✅ Match Info — static لتجنب symbol pollution
// ═══════════════════════════════════════════════════════════════
struct MatchInfo {
    bool set;
    string Tier;
    int32_t arg10;
    int32_t arg11;
};

static MatchInfo lastMatchInfo = { false, "", 0, 0 };

// ═══════════════════════════════════════════════════════════════
// ✅ StartMatch Hook — مع تحقق
// ═══════════════════════════════════════════════════════════════
DEFINE(void*, StartMatch, ptr arg1, int64_t arg2, string arg3, int64_t arg4, int32_t arg5, int32_t arg6, int64_t arg7, int32_t arg8, char arg9, int32_t arg10, int32_t arg11) {
    LOGI("StartMatch");
    LOGI("Tier %s", arg3.c_str());
    LOGI("arg4 %p, arg5 %p, arg6 %p, arg7 %p, arg8 %p, arg9 %p, arg10 %p, arg11 %p", 
         arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
    
    // ✅ احفظ معلومات آخر مباراة (لو الـ Tier صالح)
    if (!arg3.empty()) {
        lastMatchInfo.set = true;
        lastMatchInfo.Tier = arg3;
        lastMatchInfo.arg10 = arg10;
        lastMatchInfo.arg11 = arg11;
        LOGI("lastMatchInfo saved: Tier=%s arg10=%d arg11=%d", 
             arg3.c_str(), arg10, arg11);
    } else {
        LOGI("StartMatch: arg3 empty — skip saving");
    }

    return _StartMatch(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}

// ═══════════════════════════════════════════════════════════════
// ✅ جدول الرهانات — const static
// ═══════════════════════════════════════════════════════════════
static const std::map<string, int64_t> modeBets = {
    {"M1", 50},
    {"M2", 100},
    {"M3", 500},          // Call Pocket on 8 Ball
    {"M4", 2500},
    {"M5", 10000},        // Call Pocket on All Shots
    {"M6", 50000},
    {"M7", 100000},
    {"M8", 250000},
    {"M9", 500000},
    {"M10", 1000000},     // Call Pocket on 8 Ball
    {"M11", 2500000},     // Call Pocket on All Shots
    {"M12", 4000000},     // Call Pocket on 8 Ball
    {"M13", 5000000},     // Call Pocket on All Shots
    {"M14", 10000000},    // Call Pocket on All Shots
    {"M15", 15000000},    // Call Pocket on All Shots
    {"M16", 25000000},    // Call Pocket on 8 Ball
    {"M17", 100000000},   // Call Pocket on All Shots + Cushion shot on 8 Ball
};

// ═══════════════════════════════════════════════════════════════
// ✅ StartLastMatch — مع فحوصات
// ═══════════════════════════════════════════════════════════════
void StartLastMatch() {
    LOGI("StartLastMatch");
    
    // ✅ فحص sharedMenuManager
    if (!sharedMenuManager.instance) {
        LOGE("StartLastMatch: sharedMenuManager.instance is null!");
        return;
    }
    
    int mode = persistent_int["iAutoQueue_Mode"];
    
    // ═════════════════════════════════════════════════════════
    // الوضع 0: آخر مباراة
    // ═════════════════════════════════════════════════════════
    if (mode == 0) {
        if (!lastMatchInfo.set) {
            LOGW("StartLastMatch: no last match info — fallback to M1");
            // ✅ fallback: العب M1 بأرخص رهان
            _StartMatch(sharedMenuManager.instance, 0, "M1", 0, 0, 0, 0, 0, 0, 0x7100000001, 0xffffffff);
            return;
        }
        
        LOGI("StartLastMatch: replay Tier=%s arg10=%d arg11=%d",
             lastMatchInfo.Tier.c_str(), lastMatchInfo.arg10, lastMatchInfo.arg11);
        
        _StartMatch(sharedMenuManager.instance, 0, lastMatchInfo.Tier, 
                    0, 0, 0, 0, 0, 0, 
                    lastMatchInfo.arg10, lastMatchInfo.arg11);
        return;
    }
    
    // ═════════════════════════════════════════════════════════
    // الوضع 1: نسبة من الفلوس
    // ═════════════════════════════════════════════════════════
    if (mode == 1) {
        int64_t coins = sharedUserInfo.coins();
        if (coins <= 0) {
            LOGE("StartLastMatch: coins = %lld — abort", coins);
            return;
        }
        
        // ✅ فحص النسبة
        int percent = persistent_int["iAutoQueue_BetPercent"];
        if (percent <= 0) percent = 5;    // ✅ افتراضي 5%
        if (percent > 100) percent = 100; // ✅ cap
        
        int64_t maxBet = (coins * percent) / 100;
        
        LOGI("StartLastMatch: coins=%lld percent=%d%% maxBet=%lld",
             coins, percent, maxBet);
        
        // ✅ ابحث عن أفضل وضع
        string selectedMode = "M1";
        int64_t selectedBet = 50;
        bool found = false;
        
        for (const auto& [m, bet] : modeBets) {
            if (maxBet >= bet) {
                selectedMode = m;
                selectedBet = bet;
                found = true;
            }
        }
        
        // ✅ لو ما لقى (الفلوس < 50) → لا يبدأ
        if (!found) {
            LOGW("StartLastMatch: not enough coins (%lld < 50) — abort", coins);
            return;
        }
        
        LOGI("StartLastMatch: selected %s with bet %lld", 
             selectedMode.c_str(), selectedBet);
        
        _StartMatch(sharedMenuManager.instance, 0, selectedMode, 
                    0, 0, 0, 0, 0, 0, 
                    0x7100000001, 0xffffffff);
        return;
    }
    
    LOGW("StartLastMatch: unknown mode %d", mode);
}

// ═══════════════════════════════════════════════════════════════
// ✅ popMenuState Hook
// ═══════════════════════════════════════════════════════════════
DEFINE(int64_t, popMenuState, int64_t arg1, int64_t arg2, int32_t arg3, int64_t arg4) {
    LOGI("popMenuState arg1 %p, arg2 %p, arg3 %d, arg4 %p", 
         arg1, arg2, arg3, arg4);
    return _popMenuState(arg1, arg2, arg3, arg4);
}

// ═══════════════════════════════════════════════════════════════
// ✅ PopMenuState — مع فحص
// ═══════════════════════════════════════════════════════════════
void PopMenuState(int stateId) {
    LOGI("PopMenuState %d", stateId);
    
    // ✅ فحص sharedMenuManager
    if (!sharedMenuManager.instance) {
        LOGE("PopMenuState: sharedMenuManager.instance is null!");
        return;
    }
    
    // ✅ فحص stateId
    if (stateId <= 0) {
        LOGW("PopMenuState: invalid stateId %d", stateId);
        return;
    }
    
    auto _popMenuState_fn = M(int64_t, libmain + 0x3051f00, 
                              int64_t, int64_t, int32_t, int64_t);
    
    if (!_popMenuState_fn) {
        LOGE("PopMenuState: function pointer is null!");
        return;
    }
    
    _popMenuState_fn(sharedMenuManager.instance, 0, stateId, sharedMenuManager.instance);
}