#include <unistd.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <dlfcn.h>
#include <EGL/egl.h> // Header untuk EGL

#include "zygisk.hpp"
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Anubis-Mem-Tool-GUI", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// --- Status & Variabel Global untuk GUI ---
bool g_IsGuiInitialized = false;
bool g_ShowMenu = true;
int g_ScreenWidth = 0;
int g_ScreenHeight = 0;

// === PINDAHKAN VARIABEL KE SINI ===
bool g_NoRecoil = true;
bool g_RapidFire = false;
// ===================================

// --- Pointer ke Fungsi eglSwapBuffers Asli ---
EGLBoolean (*original_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);

// --- Hook untuk eglSwapBuffers ---
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // Tahap Inisialisasi (hanya berjalan sekali)
    if (!g_IsGuiInitialized) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_ScreenWidth);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_ScreenHeight);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        ImGui::StyleColorsDark();

        ImGui_ImplAndroid_Init(nullptr);
        ImGui_ImplOpenGL3_Init("#version 300 es");

        io.Fonts->AddFontDefault();

        LOGD("ImGui Initialized! Screen size: %d x %d", g_ScreenWidth, g_ScreenHeight);
        g_IsGuiInitialized = true;
    }

    // --- Loop Render ImGui (berjalan setiap frame) ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // Gambar menu kita
    if (g_ShowMenu) {
        ImGui::Begin("Anubis-Mem-Tool");
        ImGui::Text("Hello from Zygisk ImGui!");
        // Baris ini sekarang akan berfungsi dengan benar
        ImGui::Checkbox("No Recoil", &g_NoRecoil);
        ImGui::Checkbox("Rapid Fire", &g_RapidFire);
        ImGui::End();
    }

    // Render frame Dear ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Panggil fungsi asli untuk menampilkan frame ke layar
    return original_eglSwapBuffers(dpy, surface);
}


void do_hooking_thread() {
    LOGD("Hooking thread started. Waiting for EGL...");
    std::this_thread::sleep_for(std::chrono::seconds(10));

    void* egl_handle = dlopen("libEGL.so", RTLD_LAZY);
    if (!egl_handle) {
        LOGD("FATAL: Could not open libEGL.so");
        return;
    }

    void* swap_buffers_addr = dlsym(egl_handle, "eglSwapBuffers");
    if (!swap_buffers_addr) {
        LOGD("FATAL: Could not find eglSwapBuffers symbol");
        dlclose(egl_handle);
        return;
    }

    int result = DobbyHook(swap_buffers_addr, (void*)hooked_eglSwapBuffers, (void**)&original_eglSwapBuffers);
    if (result == 0) {
        LOGD("SUCCESS: eglSwapBuffers hooked at %p", swap_buffers_addr);
    } else {
        LOGD("FAILED: DobbyHook for eglSwapBuffers failed with code %d", result);
    }
}

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
            LOGD("Target application detected. Starting GUI hooking thread.");
            std::thread(do_hooking_thread).detach();
        }
        this->env->ReleaseStringUTFChars(app_name_jstring, app_name);
    }
private:
    Api *api;
    JNIEnv *env;
    // HAPUS VARIABEL DARI SINI
};

REGISTER_ZYGISK_MODULE(AnubisMemToolModule)