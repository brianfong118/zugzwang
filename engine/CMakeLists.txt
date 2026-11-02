cmake_minimum_required(VERSION 3.25)
project(zugzwang VERSION 0.2.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MinGW GCC has no ASan, so the sanitiser build is Linux-only and CI is where it runs.
set(kSanitize $<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Linux>>)

add_library(zugzwang_options INTERFACE)
# UBSan only prints by default, so -fno-sanitize-recover is what makes undefined behaviour fail CI.
target_compile_options(zugzwang_options INTERFACE
    -Wall -Wextra
    $<$<CONFIG:Release>:-march=native>
    $<${kSanitize}:-fsanitize=address,undefined;-fno-sanitize-recover=all;-fno-omit-frame-pointer>)
# Static runtimes on Windows, because the binary is copied next to lichess-bot, away from MSYS2.
target_link_options(zugzwang_options INTERFACE
    $<$<BOOL:${MINGW}>:-static>
    $<${kSanitize}:-fsanitize=address,undefined>)

add_library(zugzwang_core STATIC
    src/attacks.cpp
    src/bench.cpp
    src/eval.cpp
    src/movegen.cpp
    src/perft.cpp
    src/position.cpp
    src/search.cpp
    src/uci.cpp)
target_include_directories(zugzwang_core PUBLIC src)
target_link_libraries(zugzwang_core PUBLIC zugzwang_options)
target_compile_definitions(zugzwang_core PUBLIC ZUGZWANG_VERSION="${PROJECT_VERSION}")

add_executable(zugzwang src/main.cpp)
target_link_libraries(zugzwang PRIVATE zugzwang_core)

include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW ON
    SYSTEM)
FetchContent_MakeAvailable(Catch2)

add_executable(zugzwang_tests
    tests/main.cpp
    tests/attacks_test.cpp
    tests/bench_test.cpp
    tests/eval_test.cpp
    tests/movegen_test.cpp
    tests/perft_test.cpp
    tests/position_test.cpp
    tests/search_test.cpp
    tests/uci_test.cpp)
target_link_libraries(zugzwang_tests PRIVATE zugzwang_core Catch2::Catch2)
