cmake_minimum_required(VERSION 3.21)

cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0092 NEW)

project(LingmoConfig
    VERSION 1.0.0
    DESCRIPTION "Lingmo Desktop - Configuration system"
    HOMEPAGE_URL "https://lingmo.org"
    LANGUAGES CXX
)

# ── C++ Standard ──────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Install Dirs ──────────────────────────────────────────
include(GNUInstallDirs)

# ── Compiler Settings ─────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LingmoCompilerSettings)
include(LingmoSanitizers)
include(LingmoPCH)

# ── Options ───────────────────────────────────────────────
option(BUILD_TESTING "Build tests" ON)
option(LINGMO_ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(LINGMO_ENABLE_UNITY_BUILD "Enable unity builds" OFF)

# ── Dependencies ──────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 COMPONENTS Test QUIET)

# ── Subdirectories ────────────────────────────────────────
add_subdirectory(src)

if(BUILD_TESTING AND Qt6Test_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()
