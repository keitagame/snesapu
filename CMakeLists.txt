cmake_minimum_required(VERSION 3.15)
project(SNESAPU_Port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(snesapu SHARED
    src/Spc700.cpp
    src/Dsp.cpp
    src/SpcEngine.cpp
    src/SNESAPU.cpp
)

target_include_directories(snesapu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(snesapu PRIVATE SNESAPU_BUILD_DLL)

if(UNIX AND NOT APPLE)
    target_link_libraries(snesapu PRIVATE m)
endif()

# 静的ライブラリ版 (テスト/CLIツール用)
add_library(snesapu_static STATIC
    src/Spc700.cpp
    src/Dsp.cpp
    src/SpcEngine.cpp
    src/SNESAPU.cpp
)
target_include_directories(snesapu_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 簡易CLIプレイヤー (WAV出力) - 動作確認用
add_executable(spc2wav tools/spc2wav.cpp)
target_link_libraries(spc2wav PRIVATE snesapu_static)
if(UNIX AND NOT APPLE)
    target_link_libraries(spc2wav PRIVATE m)
endif()