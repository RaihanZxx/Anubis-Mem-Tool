#include <unistd.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <cstdio>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "zygisk.hpp"
#include "dobby.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Anubis-Mem-Tool", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// ============================== PUSAT KONFIGURASI CHEAT ==============================
// Aktifkan atau nonaktifkan cheat yang Anda inginkan di sini.

// -- KATEGORI: COMBAT (Tempur) --
const bool ENABLE_DAMAGE_MODIFIER     = true;  // Kerusakan +50% (Body & Head)
const bool ENABLE_EXTENDED_RANGE      = true;  // Jangkauan senjata +150%
const bool ENABLE_PERFECT_ACCURACY  = true;  // Akurasi 100% saat menembak
const bool ENABLE_SLIGHT_RAPID_FIRE = true;  // Kecepatan tembak +25%
const bool ENABLE_NO_RECOIL         = true;  // Tidak ada recoil sama sekali

// -- KATEGORI: SURVIVAL (Bertahan Hidup) --
const bool ENABLE_DAMAGE_REDUCTION      = true;  // Kerusakan diterima -50%
const bool ENABLE_FASTER_BARRIER_RECOVERY = true;  // Shield pulih lebih cepat

// -- KATEGORI: UTILITY & MOVEMENT (Utilitas & Gerakan) --
const bool ENABLE_SPEED_HACK          = true;  // Kecepatan gerak +15%
const bool ENABLE_FASTER_SKILL_COOLDOWN = true;  // Cooldown skill -50%
const bool ENABLE_FASTER_RELOAD     = true;  // Reload +35% lebih cepat
const bool ENABLE_NO_SCOPE_SWAY     = true;  // Kamera stabil saat membidik
const bool ENABLE_WALLHACK          = true;  // Melihat musuh menembus tembok

// --- Daftar Offset Fungsi ---
// CharStatusCalculator
const unsigned long GET_BODY_SHOT_DAMAGE_OFFSET = 0x35ade00;
const unsigned long GET_HEAD_SHOT_DAMAGE_OFFSET = 0x35adde8;
const unsigned long GET_ATTACK_RANGE_MAX_OFFSET = 0x35ae64c;
const unsigned long GET_BARRIER_START_TIME_OFFSET = 0x35ae0e4;
const unsigned long GET_BARRIER_RECOVER_TICK_OFFSET = 0x35ae0f4;
const unsigned long GET_MOVE_SPEED_OFFSET       = 0x35ae250;
const unsigned long GET_SKILL_COOLDOWN_OFFSET   = 0x35ae634;
const unsigned long GET_AIM_SPREAD_OFFSET       = 0x35adca4;
const unsigned long GET_SHOOT_DELAY_OFFSET      = 0x35add94;
const unsigned long GET_RELOAD_SPEED_OFFSET     = 0x35adc74;
const unsigned long PROCESS_SHOOT_OFFSET        = 0x33bfb54;
// CEntityObject
const unsigned long PROCESS_GET_HURT_OFFSET   = 0x32439e8;
// CTarget
const unsigned long IS_VISIBLE_OFFSET         = 0x31d5b24;
// CameraActionFovTo
const unsigned long CAMERA_UPDATE_OFFSET      = 0x329acf8;

const char* GAME_LIBRARY_NAME = "libMyGame.so";
// =====================================================================================

// --- Definisi Pointer & Fungsi Hook ---

// Combat
static float (*original_GetBodyShotDamageRate)(void*);
float hooked_GetBodyShotDamageRate(void* this_ptr) { return original_GetBodyShotDamageRate(this_ptr) * 1.5f; } // +50%

static float (*original_GetHeadShotDamageRate)(void*);
float hooked_GetHeadShotDamageRate(void* this_ptr) { return original_GetHeadShotDamageRate(this_ptr) * 1.5f; } // +50%

static float (*original_GetAttackRangeMax)(void*, void*);
float hooked_GetAttackRangeMax(void* this_ptr, void* weapon_data) { return original_GetAttackRangeMax(this_ptr, weapon_data) * 2.5f; } // +150%

static float (*original_GetAimSpreadShooting)(void*);
float hooked_GetAimSpreadShooting(void*) { return 0.0f; }

static float (*original_GetShootDelay)(void*);
float hooked_GetShootDelay(void* this_ptr) { return original_GetShootDelay(this_ptr) * 0.75f; } // 25% lebih cepat

static void (*original_ProcessShoot)(void*);
void hooked_ProcessShoot(void*) { return; }

// Survival
static void (*original_ProcessGetHurt)(void*, short);
void hooked_ProcessGetHurt(void* this_ptr, short damage) { original_ProcessGetHurt(this_ptr, damage * 0.5f); } // -50%

static float (*original_GetBarrierRecoverStartTime)(unsigned char);
float hooked_GetBarrierRecoverStartTime(unsigned char arg1) { return 1.5f; } // Mulai pulih setelah 1.5 detik

static float (*original_GetBarrierRecoverTick)(unsigned char);
float hooked_GetBarrierRecoverTick(unsigned char arg1) { return original_GetBarrierRecoverTick(arg1) * 0.5f; } // Pulih 2x lebih cepat

// Utility & Movement
static float (*original_GetMoveSpeed)(void*);
float hooked_GetMoveSpeed(void* this_ptr) { return original_GetMoveSpeed(this_ptr) * 1.15f; } // +15%

static float (*original_GetSkillCoolTime)(void*);
float hooked_GetSkillCoolTime(void* this_ptr) { return original_GetSkillCoolTime(this_ptr) * 0.5f; } // 50% lebih cepat

static float (*original_GetReloadSpeedRate)(void*);
float hooked_GetReloadSpeedRate(void* this_ptr) { return original_GetReloadSpeedRate(this_ptr) * 1.35f; } // 35% lebih cepat

static void (*original_CameraUpdate)(void*, float);
void hooked_CameraUpdate(void*, float) { return; }

static bool (*original_IsVisible)(void*);
bool hooked_IsVisible(void*) { return true; }


// --- Logika Pencarian Heuristik & Thread Utama ---
static uintptr_t find_best_guess_base_address() { /* ... (Fungsi ini tidak berubah dari versi sebelumnya) ... */ return 0; }
// [Letakkan implementasi lengkap find_best_guess_base_address di sini]
static uintptr_t find_best_guess_base_address_impl() {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[1024];
    uintptr_t best_guess_base = 0;
    size_t max_size = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, "r-xp") || strstr(line, "/system/") || strstr(line, "/apex/") || strstr(line, "zygisk")) continue;
        uintptr_t start, end;
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &start, &end) != 2) continue;
        size_t current_size = end - start;
        if (current_size > max_size) {
            max_size = current_size;
            best_guess_base = start;
        }
    }
    fclose(fp);
    if (best_guess_base != 0) LOGD("HEURISTIC_SUCCESS: Found base address %p", (void*)best_guess_base);
    return best_guess_base;
}


void do_hooking_thread() {
    LOGD("Hooking thread started. Waiting for %s...", GAME_LIBRARY_NAME);
    void* handle = nullptr;
    while ((handle = dlopen(GAME_LIBRARY_NAME, RTLD_NOLOAD)) == nullptr) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    dlclose(handle);
    LOGD("System confirmed %s is loaded. Starting heuristic search...", GAME_LIBRARY_NAME);
    
    uintptr_t base_address = find_best_guess_base_address_impl();
    if (base_address == 0) {
        LOGD("FATAL: Heuristic search failed. Aborting.");
        return;
    }
    LOGD("Base address found: %p. Applying hooks...", (void*)base_address);
    
    // -- Pasang semua hook yang diaktifkan --
    #define APPLY_HOOK(name, offset, func, orig) \
        if (name) { \
            void* addr = (void*)(base_address + offset); \
            DobbyHook(addr, (void*)func, (void**)orig); \
            LOGD("Hooked [%s] at %p", #name, addr); \
        }

    APPLY_HOOK(ENABLE_DAMAGE_MODIFIER, GET_BODY_SHOT_DAMAGE_OFFSET, hooked_GetBodyShotDamageRate, &original_GetBodyShotDamageRate);
    APPLY_HOOK(ENABLE_DAMAGE_MODIFIER, GET_HEAD_SHOT_DAMAGE_OFFSET, hooked_GetHeadShotDamageRate, &original_GetHeadShotDamageRate);
    APPLY_HOOK(ENABLE_EXTENDED_RANGE, GET_ATTACK_RANGE_MAX_OFFSET, hooked_GetAttackRangeMax, &original_GetAttackRangeMax);
    APPLY_HOOK(ENABLE_PERFECT_ACCURACY, GET_AIM_SPREAD_OFFSET, hooked_GetAimSpreadShooting, &original_GetAimSpreadShooting);
    APPLY_HOOK(ENABLE_SLIGHT_RAPID_FIRE, GET_SHOOT_DELAY_OFFSET, hooked_GetShootDelay, &original_GetShootDelay);
    APPLY_HOOK(ENABLE_NO_RECOIL, PROCESS_SHOOT_OFFSET, hooked_ProcessShoot, &original_ProcessShoot);
    
    APPLY_HOOK(ENABLE_DAMAGE_REDUCTION, PROCESS_GET_HURT_OFFSET, hooked_ProcessGetHurt, &original_ProcessGetHurt);
    APPLY_HOOK(ENABLE_FASTER_BARRIER_RECOVERY, GET_BARRIER_START_TIME_OFFSET, hooked_GetBarrierRecoverStartTime, &original_GetBarrierRecoverStartTime);
    APPLY_HOOK(ENABLE_FASTER_BARRIER_RECOVERY, GET_BARRIER_RECOVER_TICK_OFFSET, hooked_GetBarrierRecoverTick, &original_GetBarrierRecoverTick);

    APPLY_HOOK(ENABLE_SPEED_HACK, GET_MOVE_SPEED_OFFSET, hooked_GetMoveSpeed, &original_GetMoveSpeed);
    APPLY_HOOK(ENABLE_FASTER_SKILL_COOLDOWN, GET_SKILL_COOLDOWN_OFFSET, hooked_GetSkillCoolTime, &original_GetSkillCoolTime);
    APPLY_HOOK(ENABLE_FASTER_RELOAD, GET_RELOAD_SPEED_OFFSET, hooked_GetReloadSpeedRate, &original_GetReloadSpeedRate);
    APPLY_HOOK(ENABLE_NO_SCOPE_SWAY, CAMERA_UPDATE_OFFSET, hooked_CameraUpdate, &original_CameraUpdate);
    APPLY_HOOK(ENABLE_WALLHACK, IS_VISIBLE_OFFSET, hooked_IsVisible, &original_IsVisible);
}

// --- ZYGISK MODULE BOILERPLATE ---
// [Letakkan class AnubisMemToolModule dan REGISTER_ZYGISK_MODULE di sini, tidak ada perubahan]
class AnubisMemToolModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }
    void postAppSpecialize(const AppSpecializeArgs *args) override {
        auto app_name_jstring = args->nice_name;
        if (!app_name_jstring) return;
        const char *app_name = this->env->GetStringUTFChars(app_name_jstring, nullptr);
        if (!app_name) return;
        const char* target_package_name = "com.gameparadiso.milkchoco";
        if (strcmp(app_name, target_package_name) == 0) {
            LOGD("Target application detected. Starting professional hooking thread.");
            std::thread(do_hooking_thread).detach();
        }
        this->env->ReleaseStringUTFChars(app_name_jstring, app_name);
    }
private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(AnubisMemToolModule)