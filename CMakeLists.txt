cmake_minimum_required(VERSION 3.20)
project(RPGNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RPGNative WIN32
  main.cpp
  engine/Engine.cpp
  engine/Renderer.cpp
  platform/Win32Window.cpp
  game/Game.cpp
)

target_include_directories(RPGNative PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(RPGNative PRIVATE UNICODE _UNICODE NOMINMAX)

if (MSVC)
  target_compile_options(RPGNative PRIVATE /W4 /EHsc)
endif()

target_link_libraries(RPGNative PRIVATE d3d11 dxgi dxguid)