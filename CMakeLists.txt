cmake_minimum_required(VERSION 3.20)
project(RPGNative LANGUAGES CXX)

if (NOT WIN32)
  message(FATAL_ERROR "Mediterranean requires Windows and DirectX 11.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RPGNative WIN32
  main.cpp
  engine/Engine.cpp
  engine/Renderer.cpp
  platform/Win32Window.cpp
  game/Game.cpp
  game/Simulation.cpp
)

target_include_directories(RPGNative PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(RPGNative PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_definitions(RPGNative PRIVATE RPG_SAVE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/saves")

if (MSVC)
  target_compile_options(RPGNative PRIVATE /W4 /EHsc)
endif()

target_link_libraries(RPGNative PRIVATE d3d11 dxgi dxguid d3dcompiler user32 gdi32)

include(CTest)
if (BUILD_TESTING)
  add_executable(SimulationTests tests/SimulationTests.cpp game/Simulation.cpp)
  target_include_directories(SimulationTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME SimulationTests COMMAND SimulationTests)
endif()
