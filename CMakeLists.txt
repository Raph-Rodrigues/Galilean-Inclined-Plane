cmake_minimum_required(VERSION 3.24)
project(galileu LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# --- SDL3 (usa o do sistema se existir, senão baixa) ---
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.0   # pode subir para uma 3.2.x mais recente
    FIND_PACKAGE_ARGS 3.2 CONFIG
)

# --- Lua (o repositório não tem CMakeLists, então montamos a lib) ---
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.7
)

# --- Corrosion (integra Cargo ao CMake) ---
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG        v0.5
)

FetchContent_MakeAvailable(SDL3 lua Corrosion)

# Lua como biblioteca estática
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/*.c)
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*/(lua|luac|onelua)\\.c$")
add_library(lua_lib STATIC ${LUA_SOURCES})
target_include_directories(lua_lib PUBLIC ${lua_SOURCE_DIR})
if(UNIX)
    target_compile_definitions(lua_lib PRIVATE LUA_USE_POSIX)
    target_link_libraries(lua_lib PUBLIC m ${CMAKE_DL_LIBS})
endif()

# Crate Rust -> alvo CMake "physics"
corrosion_import_crate(MANIFEST_PATH physics/Cargo.toml)

# Executável
add_executable(galileu cpp/main.cpp)
target_compile_definitions(galileu PRIVATE
    GALILEU_SCRIPTS_DIR="${CMAKE_SOURCE_DIR}/scripts")
target_link_libraries(galileu PRIVATE SDL3::SDL3 lua_lib physics)
