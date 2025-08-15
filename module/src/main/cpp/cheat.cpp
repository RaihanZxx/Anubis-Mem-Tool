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
// -- CHEAT DASAR / QoL --
const bool ENABLE_NO_RECOIL         = true;
const bool ENABLE_FASTER_RELOAD     = true;
const bool ENABLE_PERFECT_ACCURACY  = true;
const bool ENABLE_SLIGHT_RAPID_FIRE = true;
const bool ENABLE_NO_SCOPE_SWAY     = true;

// -- CHEAT POWERFUL (Gunakan dengan bijak) --
const bool ENABLE_WALLHACK          = true;
const bool ENABLE_DAMAGE_REDUCTION  = true;

// -- CHEAT BARU DARI GAMESCENE --
const bool ENABLE_DAMAGE_BOOST      = true;  // Damage +15%
const bool ENABLE_FORCE_AIM_ASSIST  = true;  // Aim assist selalu aktif
const bool ENABLE_SHOOT_INVULNERABLE= true;  // Bisa menembak musuh yang baru spawn/kebal
const bool ENABLE_CHAMS_ESP         = true;  // Placeholder untuk mewarnai musuh

// --- Daftar Offset Fungsi ---
// CharStatusCalculator
const unsigned long PROCESS_SHOOT_OFFSET      = 0x33bfb54;
const unsigned long GET_RELOAD_SPEED_OFFSET   = 0x35adc74;
const unsigned long GET_AIM_SPREAD_OFFSET     = 0x35adca4;
const unsigned long GET_SHOOT_DELAY_OFFSET    = 0x35add94;
// CEntityObject
const unsigned long PROCESS_GET_HURT_OFFSET   = 0x32439e8;
// CTarget
const unsigned long IS_VISIBLE_OFFSET         = 0x31d5b24;
// CameraActionFovTo
const unsigned long CAMERA_UPDATE_OFFSET      = 0x329acf8;
// GameScene
const unsigned long CALC_SHOOT_DMG_OFFSET     = 0x316e0c0;
const unsigned long IS_AIM_ASSIST_OFFSET      = 0x3166f30;
const unsigned long CAN_ATTACK_OFFSET         = 0x3168af0;
const unsigned long CHANGE_TEAM_COLOR_OFFSET  = 0x3165a28;

const char* GAME_LIBRARY_NAME = "libMyGame.so";
// =====================================================================================

// --- Definisi Pointer & Fungsi Hook ---

// QoL & Combat Cheats
static void (*original_ProcessShoot)(void*);
void hooked_ProcessShoot(void*) { return; }

static float (*original_GetReloadSpeedRate)(void*);
float hooked_GetReloadSpeedRate(void* this_ptr) { return original_GetReloadSpeedRate(this_ptr) * 1.8f; }

static float (*original_GetAimSpreadShooting)(void*);
float hooked_GetAimSpreadShooting(void* this_ptr) { return 0.0f; }

static float (*original_GetShootDelay)(void*);
float hooked_GetShootDelay(void* this_ptr) { return original_GetShootDelay(this_ptr) * 0.35f; }

static void (*original_ProcessGetHurt)(void*, short);
void hooked_ProcessGetHurt(void* this_ptr, short damage) { original_ProcessGetHurt(this_ptr, damage * 0.5f); }

static void (*original_CameraUpdate)(void*, float);
void hooked_CameraUpdate(void* this_ptr, float time) { return; }

// ESP Cheats
static bool (*original_IsVisible)(void*);
bool hooked_IsVisible(void* this_ptr) { return true; }

// GameScene Cheats
static float (*original_CalculateShootDamage)(void*, void*, unsigned char, float);
float hooked_CalculateShootDamage(void* this_ptr, void* user_info, unsigned char arg2, float arg3) {
    float original_damage = original_CalculateShootDamage(this_ptr, user_info, arg2, arg3);
    return original_damage * 1.15f; // Damage +15%
}

static bool (*original_IsAimAssist)(void*);
bool hooked_IsAimAssist(void* this_ptr) { return true; }

static bool (*original_CanAtk)(void*, void*, void*, int);
bool hooked_CanAtk(void* this_ptr, void* attacker, void* target, int flags) { return true; }

static void (*original_ChangeCharTeamColor)(void*, void*);
void hooked_ChangeCharTeamColor(void* this_ptr, void* user_info) {
    // Ini adalah placeholder. Logika Chams yang sebenarnya memerlukan cara
    // untuk memeriksa apakah 'user_info' adalah musuh, lalu memanggil fungsi
    // rendering untuk mengubah warna. Untuk saat ini, kita panggil saja aslinya.
    original_ChangeCharTeamColor(this_ptr, user_info);
}


// --- Logika Pencarian Heuristik & Thread Utama ---
static uintptr_t find_best_guess_base_address() {
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
    
    uintptr_t base_address = find_best_guess_base_address();
    if (base_address == 0) {
        LOGD("FATAL: Heuristic search failed. Aborting.");
        return;
    }
    LOGD("Base address found: %p. Applying hooks...", (void*)base_address);
    
#define APPLY_HOOK(name, offset, func, orig) \
    if (name) { \
        void* addr = (void*)(base_address + offset); \
        DobbyHook(addr, (void*)func, (void**)&orig); \
        LOGD("Hooked [%s] at %p", #name, addr); \
    }

    APPLY_HOOK(ENABLE_NO_RECOIL, PROCESS_SHOOT_OFFSET, hooked_ProcessShoot, original_ProcessShoot);
    APPLY_HOOK(ENABLE_FASTER_RELOAD, GET_RELOAD_SPEED_OFFSET, hooked_GetReloadSpeedRate, original_GetReloadSpeedRate);
    APPLY_HOOK(ENABLE_PERFECT_ACCURACY, GET_AIM_SPREAD_OFFSET, hooked_GetAimSpreadShooting, original_GetAimSpreadShooting);
    APPLY_HOOK(ENABLE_SLIGHT_RAPID_FIRE, GET_SHOOT_DELAY_OFFSET, hooked_GetShootDelay, original_GetShootDelay);
    APPLY_HOOK(ENABLE_DAMAGE_REDUCTION, PROCESS_GET_HURT_OFFSET, hooked_ProcessGetHurt, original_ProcessGetHurt);
    APPLY_HOOK(ENABLE_WALLHACK, IS_VISIBLE_OFFSET, hooked_IsVisible, original_IsVisible);
    APPLY_HOOK(ENABLE_NO_SCOPE_SWAY, CAMERA_UPDATE_OFFSET, hooked_CameraUpdate, original_CameraUpdate);
    APPLY_HOOK(ENABLE_DAMAGE_BOOST, CALC_SHOOT_DMG_OFFSET, hooked_CalculateShootDamage, original_CalculateShootDamage);
    APPLY_HOOK(ENABLE_FORCE_AIM_ASSIST, IS_AIM_ASSIST_OFFSET, hooked_IsAimAssist, original_IsAimAssist);
    APPLY_HOOK(ENABLE_SHOOT_INVULNERABLE, CAN_ATTACK_OFFSET, hooked_CanAtk, original_CanAtk);
    APPLY_HOOK(ENABLE_CHAMS_ESP, CHANGE_TEAM_COLOR_OFFSET, hooked_ChangeCharTeamColor, original_ChangeCharTeamColor);

#undef APPLY_HOOK
}


// --- ZYGISK MODULE BOILERPLATE ---
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