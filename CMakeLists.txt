cmake_minimum_required(VERSION 3.16)
project(juice VERSION 0.1.4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Iconv)

# core library (all logic except main)
add_library(juice_lib STATIC
    src/config.cpp
    src/charset.cpp
    src/ast.cpp
    src/sexp_writer.cpp
    src/sexp_reader.cpp
    src/charset/pc98.cpp
    src/charset/english.cpp
    src/charset/europe.cpp
    src/charset/korean.cpp
    src/charset/korean_gamebox.cpp
    src/charset/korean_hannuri.cpp
    src/charset/korean_kk.cpp
    src/charset/korean_parangsae.cpp
    src/charset/chinese.cpp
    src/engine/ai5/opener.cpp
    src/engine/ai5/parser.cpp
    src/engine/ai5/loader.cpp
    src/engine/ai5/compiler.cpp
    src/engine/ai1/opener.cpp
    src/engine/ai1/parser.cpp
    src/engine/ai1/loader.cpp
    src/engine/ai1/compiler.cpp
    src/engine/adv/opener.cpp
    src/engine/adv/parser.cpp
    src/engine/adv/loader.cpp
    src/engine/adv/compiler.cpp
)
target_include_directories(juice_lib PUBLIC src)

if(Iconv_FOUND)
    target_link_libraries(juice_lib PUBLIC Iconv::Iconv)
    target_compile_definitions(juice_lib PUBLIC HAVE_ICONV)
elseif(WIN32)
    # use vendored win-iconv on Windows when system iconv is not available
    add_library(win_iconv STATIC third_party/win_iconv.c)
    target_include_directories(win_iconv PUBLIC third_party)
    target_link_libraries(juice_lib PUBLIC win_iconv)
    target_compile_definitions(juice_lib PUBLIC HAVE_ICONV)
endif()

# main executable
add_executable(juice src/main.cpp)
target_link_libraries(juice PRIVATE juice_lib)

# statically link gcc/c++ runtime on MinGW so users don't need the dlls
if(MINGW)
    target_link_options(juice PRIVATE -static)
endif()

# tests (optional, enabled with -DBUILD_TESTING=ON)
option(BUILD_TESTING "Build unit tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
