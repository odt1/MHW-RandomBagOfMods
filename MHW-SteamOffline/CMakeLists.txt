cmake_minimum_required(VERSION 3.24)

project(MHW-SteamOffline VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# The game loads plugins with LoadLibrary; static CRT keeps the plugin self-contained.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(MHW-SteamOffline SHARED src/dllmain.cpp)

target_compile_definitions(MHW-SteamOffline PRIVATE
  WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS
  MHW_STEAMOFFLINE_VERSION="${PROJECT_VERSION}"
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
# Optimisation is set on the target, not only through CMAKE_CXX_FLAGS_<CONFIG>: a build folder first
# configured without the MSVC environment caches those as empty. /Zi and /DEBUG keep a PDB.
target_compile_options(MHW-SteamOffline PRIVATE /W4 /permissive- /utf-8 /EHsc /Zi "$<$<NOT:$<CONFIG:Debug>>:/O2;/Gw>")
target_link_options(MHW-SteamOffline PRIVATE /DEBUG /OPT:REF /OPT:ICF)
