cmake_minimum_required(VERSION 3.20)
project(RPGNative LANGUAGES CXX)

if (NOT WIN32)
  message(FATAL_ERROR "Mediterranean requires Windows and DirectX 11.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
  include(cmake/DetectMsvcIncludes.cmake)
endif()

add_library(MediterraneanSimulation STATIC game/Simulation.cpp game/Progression.cpp game/WorldMap.cpp game/Inventory.cpp game/Trade.cpp)
target_include_directories(MediterraneanSimulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(MediterraneanClient STATIC
  engine/Engine.cpp
  engine/Renderer.cpp
  engine/GpuTimer.cpp
  engine/Geometry.cpp
  engine/FontAtlas.cpp
  platform/Win32Window.cpp
  game/Game.cpp
  game/WorldView.cpp
  game/CharacterView.cpp
  game/Interface.cpp
  game/InterfaceState.cpp
  game/MapArtwork.cpp
)
add_executable(RPGNative WIN32 main.cpp)
target_link_libraries(RPGNative PRIVATE MediterraneanClient)

target_include_directories(MediterraneanClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(MediterraneanClient PUBLIC UNICODE _UNICODE NOMINMAX)
target_compile_definitions(MediterraneanClient PRIVATE RPG_SAVE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/saves")

if (MSVC)
  target_compile_options(MediterraneanClient PRIVATE /W4 /EHsc)
endif()

target_link_libraries(MediterraneanClient PUBLIC MediterraneanSimulation d3d11 dxgi dxguid d3dcompiler user32 gdi32 windowscodecs ole32 shell32 psapi)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT RPGNative)

include(CTest)
if (BUILD_TESTING)
  add_executable(InterfaceTests tests/InterfaceTests.cpp)
  target_link_libraries(InterfaceTests PRIVATE MediterraneanClient)
  target_compile_definitions(InterfaceTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME InterfaceTests COMMAND InterfaceTests)
  add_executable(GeometryTests tests/GeometryTests.cpp engine/Geometry.cpp)
  target_include_directories(GeometryTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(GeometryTests PRIVATE NOMINMAX)
  add_test(NAME GeometryTests COMMAND GeometryTests)
  add_executable(SimulationTests tests/SimulationTests.cpp)
  target_link_libraries(SimulationTests PRIVATE MediterraneanSimulation)
  add_test(NAME SimulationTests COMMAND SimulationTests)
  add_executable(NavigationTests tests/NavigationTests.cpp)
  target_link_libraries(NavigationTests PRIVATE MediterraneanSimulation)
  add_test(NAME NavigationTests COMMAND NavigationTests)
  add_executable(InventoryTests tests/InventoryTests.cpp)
  target_link_libraries(InventoryTests PRIVATE MediterraneanSimulation)
  add_test(NAME InventoryTests COMMAND InventoryTests)
  add_executable(CombatTests tests/CombatTests.cpp)
  target_link_libraries(CombatTests PRIVATE MediterraneanSimulation)
  add_test(NAME CombatTests COMMAND CombatTests)
  add_executable(ProgressionTests tests/ProgressionTests.cpp)
  target_link_libraries(ProgressionTests PRIVATE MediterraneanSimulation)
  add_test(NAME ProgressionTests COMMAND ProgressionTests)
  add_executable(TrainingTests tests/TrainingTests.cpp)
  target_link_libraries(TrainingTests PRIVATE MediterraneanSimulation)
  add_test(NAME TrainingTests COMMAND TrainingTests)
  add_executable(TradeTests tests/TradeTests.cpp)
  target_link_libraries(TradeTests PRIVATE MediterraneanSimulation)
  add_test(NAME TradeTests COMMAND TradeTests)
endif()
