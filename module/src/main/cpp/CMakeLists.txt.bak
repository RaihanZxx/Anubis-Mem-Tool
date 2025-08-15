cmake_minimum_required(VERSION 3.22.1)
project(${MODULE_NAME})

# --- Flags Kompilasi (Tidak Berubah) ---
set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

# --- TAMBAHKAN SEMUA FILE SUMBER BARU ---
# Menggunakan GLOB_RECURSE untuk secara otomatis menemukan semua file .cpp/.c
file(GLOB_RECURSE CHEAT_SOURCES
    "cheat.cpp"
    "imgui/*.cpp"
    "KittyMemory/*.cpp"
    "xdl/*.c"
)

# Buat library dari semua file sumber
add_library(${MODULE_NAME} SHARED ${CHEAT_SOURCES})

# --- TAUTKAN LIBRARY BARU YANG DIBUTUHKAN UNTUK GUI ---
# Kita butuh EGL dan GLESv2 untuk merender ImGui
target_link_libraries(${MODULE_NAME}
    cxx
    log
    dl
    EGL          # <-- PENTING UNTUK MENDAPATKAN KONTEKS RENDER
    GLESv2       # <-- PENTING UNTUK FUNGSI GAMBAR OPENGL
    ${CMAKE_CURRENT_SOURCE_DIR}/dobby/${CMAKE_ANDROID_ARCH_ABI}/libdobby.a
)

# --- TAMBAHKAN SEMUA DIREKTORI INCLUDE ---
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dobby
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/KittyMemory
    ${CMAKE_CURRENT_SOURCE_DIR}/xdl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Struct
)

# Subdirektori external (tidak berubah)
add_subdirectory(external)