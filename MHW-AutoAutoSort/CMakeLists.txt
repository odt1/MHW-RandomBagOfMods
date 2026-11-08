cmake_minimum_required(VERSION 3.25)

project(MHWAutoAutoSort VERSION 0.9.0 LANGUAGES CXX)
set(AUTO_AUTO_SORT_VERSION "${PROJECT_VERSION}")

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "MHW Auto-Auto-Sort requires Windows x64 and MSVC.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(auto_auto_sort_target_defaults target)
    set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN
        AUTO_AUTO_SORT_VERSION="${AUTO_AUTO_SORT_VERSION}")
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /EHsc /Zi)
    target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/O2>)
    target_link_options(${target} PRIVATE /DEBUG)
endfunction()

find_package(minhook CONFIG REQUIRED)

add_library(MHW-AutoAutoSort SHARED
    src/dllmain.cpp
    src/game/autosort.cpp
    src/game/bootstrap.cpp
    src/game/menus.cpp
    src/runtime/log.cpp
    src/runtime/memory.cpp
    src/runtime/profile.cpp
)

target_link_libraries(MHW-AutoAutoSort PRIVATE bcrypt minhook::minhook)
target_include_directories(MHW-AutoAutoSort PRIVATE src)
auto_auto_sort_target_defaults(MHW-AutoAutoSort)
set_target_properties(MHW-AutoAutoSort PROPERTIES
    OUTPUT_NAME "MHW-AutoAutoSort"
    PDB_NAME "MHW-AutoAutoSort"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_memory
        tests/test_memory.cpp
        src/runtime/memory.cpp
    )
    target_include_directories(test_memory PRIVATE src)
    auto_auto_sort_target_defaults(test_memory)
    add_test(NAME memory COMMAND test_memory)

    add_executable(test_profile tests/test_profile.cpp src/runtime/profile.cpp)
    target_include_directories(test_profile PRIVATE src)
    target_link_libraries(test_profile PRIVATE bcrypt)
    auto_auto_sort_target_defaults(test_profile)
    add_test(NAME profile COMMAND test_profile)

    add_executable(test_bootstrap tests/test_bootstrap.cpp src/runtime/profile.cpp)
    target_include_directories(test_bootstrap PRIVATE src)
    target_link_libraries(test_bootstrap PRIVATE bcrypt)
    auto_auto_sort_target_defaults(test_bootstrap)
    add_dependencies(test_bootstrap MHW-AutoAutoSort)
    add_test(NAME bootstrap COMMAND test_bootstrap $<TARGET_FILE:MHW-AutoAutoSort>)
    set_tests_properties(bootstrap PROPERTIES TIMEOUT 30)

    add_executable(test_pouch_order tests/test_pouch_order.cpp)
    target_include_directories(test_pouch_order PRIVATE src)
    auto_auto_sort_target_defaults(test_pouch_order)
    add_test(NAME pouch_order COMMAND test_pouch_order)

    find_program(PWSH_EXECUTABLE NAMES pwsh REQUIRED)
    add_test(NAME deploy
        COMMAND "${PWSH_EXECUTABLE}" -NoProfile -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_deploy.ps1"
    )
endif()
