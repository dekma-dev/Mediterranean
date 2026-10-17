cmake_minimum_required(VERSION 3.20)
project(RPGNative LANGUAGES CXX)

if (NOT WIN32)
  message(FATAL_ERROR "Mediterranean requires Windows and DirectX 11.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(MediterraneanSimulation STATIC game/Simulation.cpp game/WorldMap.cpp)
target_include_directories(MediterraneanSimulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(RPGNative WIN32
  main.cpp
  engine/Engine.cpp
  engine/Renderer.cpp
  platform/Win32Window.cpp
  game/Game.cpp
  game/WorldView.cpp
)

target_include_directories(RPGNative PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(RPGNative PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_definitions(RPGNative PRIVATE RPG_SAVE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/saves")

if (MSVC)
  target_compile_options(RPGNative PRIVATE /W4 /EHsc)
endif()

target_link_libraries(RPGNative PRIVATE MediterraneanSimulation d3d11 dxgi dxguid d3dcompiler user32 gdi32 windowscodecs ole32 shell32)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT RPGNative)

include(CTest)
if (BUILD_TESTING)
  add_executable(SimulationTests tests/SimulationTests.cpp)
  target_link_libraries(SimulationTests PRIVATE MediterraneanSimulation)
  add_test(NAME SimulationTests COMMAND SimulationTests)
  add_executable(NavigationTests tests/NavigationTests.cpp)
  target_link_libraries(NavigationTests PRIVATE MediterraneanSimulation)
  add_test(NAME NavigationTests COMMAND NavigationTests)
  add_executable(InventoryTests tests/InventoryTests.cpp)
  target_link_libraries(InventoryTests PRIVATE MediterraneanSimulation)
  add_test(NAME InventoryTests COMMAND InventoryTests)
endif()
