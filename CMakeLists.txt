cmake_minimum_required(VERSION 3.20)
project(RPGNative LANGUAGES CXX)

if (NOT WIN32)
  message(FATAL_ERROR "Mediterranean requires Windows and DirectX 11.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep compiler debug records in object files instead of a shared PDB server.
# The linker still emits the executable PDB for Visual Studio debugging. This
# also permits local builds when mspdbsrv cannot be reached (MSVC C1902).
if(MSVC)
  foreach(config DEBUG RELWITHDEBINFO)
    string(REGEX REPLACE "/Z[iI]" "/Z7" CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
  endforeach()
endif()

if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
  include(cmake/DetectMsvcIncludes.cmake)
endif()

include(cmake/QuestText.cmake)
generate_quest_text("${CMAKE_CURRENT_BINARY_DIR}/generated/QuestText.generated.h")

add_library(MediterraneanSimulation STATIC game/Simulation.cpp game/Progression.cpp game/WorldMap.cpp game/Inventory.cpp game/Trade.cpp game/Quests.cpp)
target_include_directories(MediterraneanSimulation PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_include_directories(MediterraneanSimulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(MediterraneanClient STATIC
  engine/Engine.cpp
  engine/Audio.cpp
  engine/Renderer.cpp
  engine/GpuTimer.cpp
  engine/Geometry.cpp
  engine/Skinning.cpp
  engine/FontAtlas.cpp
  platform/Win32Window.cpp
  game/Game.cpp
  game/WorldView.cpp
  game/LandmarkView.cpp
  game/CharacterView.cpp
  game/CharacterPose.cpp
  game/CharacterRig.cpp
  game/CombatEffects.cpp
  game/Interface.cpp
  game/InterfaceState.cpp
  game/MapArtwork.cpp
  game/TerrainSurface.cpp
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

target_link_libraries(MediterraneanClient PUBLIC MediterraneanSimulation d3d11 dxgi dxguid d3dcompiler user32 gdi32 windowscodecs ole32 shell32 psapi xaudio2)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT RPGNative)

include(CTest)
if (BUILD_TESTING)
  add_executable(SkinningTests tests/SkinningTests.cpp)
  target_link_libraries(SkinningTests PRIVATE MediterraneanClient)
  add_test(NAME SkinningTests COMMAND SkinningTests)
  add_executable(AudioTests tests/AudioTests.cpp)
  target_link_libraries(AudioTests PRIVATE MediterraneanClient)
  add_test(NAME AudioTests COMMAND AudioTests)
  add_executable(FootPlacementTests tests/FootPlacementTests.cpp)
  target_link_libraries(FootPlacementTests PRIVATE MediterraneanClient)
  add_test(NAME FootPlacementTests COMMAND FootPlacementTests)
  add_executable(CombatEffectsTests tests/CombatEffectsTests.cpp)
  target_link_libraries(CombatEffectsTests PRIVATE MediterraneanClient)
  add_test(NAME CombatEffectsTests COMMAND CombatEffectsTests)
  add_executable(AnimationTests tests/AnimationTests.cpp)
  target_link_libraries(AnimationTests PRIVATE MediterraneanClient)
  add_test(NAME AnimationTests COMMAND AnimationTests)
  add_executable(ShadowTests tests/ShadowTests.cpp)
  target_link_libraries(ShadowTests PRIVATE MediterraneanClient)
  add_test(NAME ShadowTests COMMAND ShadowTests)
  add_executable(MotionTests tests/MotionTests.cpp)
  target_link_libraries(MotionTests PRIVATE MediterraneanClient)
  add_test(NAME MotionTests COMMAND MotionTests)
  add_executable(InterfaceTests tests/InterfaceTests.cpp)
  target_link_libraries(InterfaceTests PRIVATE MediterraneanClient)
  target_compile_definitions(InterfaceTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME InterfaceTests COMMAND InterfaceTests)
  add_executable(GeometryTests tests/GeometryTests.cpp engine/Geometry.cpp engine/Skinning.cpp game/CharacterPose.cpp)
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
  add_executable(TargetDashTests tests/TargetDashTests.cpp)
  target_link_libraries(TargetDashTests PRIVATE MediterraneanSimulation)
  add_test(NAME TargetDashTests COMMAND TargetDashTests)
  add_executable(CombatTests tests/CombatTests.cpp)
  target_link_libraries(CombatTests PRIVATE MediterraneanSimulation)
  add_test(NAME CombatTests COMMAND CombatTests)
  add_executable(AttackTimelineTests tests/AttackTimelineTests.cpp)
  target_link_libraries(AttackTimelineTests PRIVATE MediterraneanSimulation)
  add_test(NAME AttackTimelineTests COMMAND AttackTimelineTests)
  add_executable(TalentTests tests/TalentTests.cpp)
  target_link_libraries(TalentTests PRIVATE MediterraneanSimulation)
  add_test(NAME TalentTests COMMAND TalentTests)
  add_executable(LoadoutTests tests/LoadoutTests.cpp)
  target_link_libraries(LoadoutTests PRIVATE MediterraneanSimulation)
  add_test(NAME LoadoutTests COMMAND LoadoutTests)
  add_executable(QuestDialogueTests tests/QuestDialogueTests.cpp)
  target_link_libraries(QuestDialogueTests PRIVATE MediterraneanSimulation)
  add_test(NAME QuestDialogueTests COMMAND QuestDialogueTests)
  add_test(NAME QuestTextTests COMMAND ${CMAKE_COMMAND}
    "-DQUEST_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/quest-text-check"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/QuestTextTests.cmake")
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
