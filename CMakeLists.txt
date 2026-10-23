cmake_minimum_required(VERSION 3.20)
project(RPGNative LANGUAGES CXX RC)

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

add_executable(SceneShaderCompiler tools/SceneShaderCompiler.cpp)
target_include_directories(SceneShaderCompiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(SceneShaderCompiler PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(SceneShaderCompiler PRIVATE d3dcompiler)
if(MSVC)
  target_compile_options(SceneShaderCompiler PRIVATE /W4 /EHsc)
endif()
set(scene_shader_header "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/SceneBytecode.generated.h")
add_custom_command(OUTPUT "${scene_shader_header}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>"
  COMMAND $<TARGET_FILE:SceneShaderCompiler> "${scene_shader_header}"
  DEPENDS SceneShaderCompiler engine/SceneShader.h
  COMMENT "Compiling embedded DirectX 11 shaders"
  VERBATIM)

add_library(ObjImport STATIC tools/ObjImport.cpp)
add_library(SkeletonRuntime STATIC engine/Skeleton.cpp)
add_library(GltfRigImport STATIC tools/GltfRigImport.cpp)
target_link_libraries(GltfRigImport PUBLIC SkeletonRuntime)
add_executable(RigAssetInspector tools/RigAssetInspector.cpp)
target_link_libraries(RigAssetInspector PRIVATE GltfRigImport)
if(MSVC)
  target_compile_options(GltfRigImport PRIVATE /W4 /EHsc)
endif()
target_include_directories(SkeletonRuntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(SkeletonRuntime PUBLIC NOMINMAX)
if(MSVC)
  target_compile_options(SkeletonRuntime PRIVATE /W4 /EHsc)
endif()
target_include_directories(ObjImport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ObjImport PUBLIC NOMINMAX)
add_executable(MeshAssetCompiler tools/MeshAssetCompiler.cpp)
target_link_libraries(MeshAssetCompiler PRIVATE ObjImport)
if(MSVC)
  target_compile_options(ObjImport PRIVATE /W4 /EHsc)
  target_compile_options(MeshAssetCompiler PRIVATE /W4 /EHsc)
endif()
set(blade_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/weapons/frontier-blade.obj")
set(blade_header "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/Blade.generated.h")
add_custom_command(OUTPUT "${blade_header}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>"
  COMMAND $<TARGET_FILE:MeshAssetCompiler> "${blade_source}" "${blade_header}" Blade
  DEPENDS MeshAssetCompiler "${blade_source}"
  COMMENT "Validating and embedding the authored blade mesh"
  VERBATIM)
# One producer shared by the client and standalone geometry checks.
add_custom_target(EmbeddedMeshes DEPENDS "${blade_header}")

add_library(MediterraneanClient STATIC
  "${scene_shader_header}"
  engine/Engine.cpp
  engine/Audio.cpp
  engine/Renderer.cpp
  engine/GpuTimer.cpp
  engine/Geometry.cpp
  engine/Skinning.cpp
  engine/HandSkinning.cpp
  engine/FontAtlas.cpp
  platform/Win32Window.cpp
  game/Game.cpp
  game/CharacterSelection.cpp
  game/ProfileStorage.cpp
  game/WorldView.cpp
  game/LandmarkView.cpp
  game/CharacterView.cpp
  game/CharacterPose.cpp
  game/HandPose.cpp
  game/CharacterRig.cpp
  game/CombatEffects.cpp
  game/ClassEffects.cpp
  game/Interface.cpp
  game/InterfaceState.cpp
  game/MapArtwork.cpp
  game/TerrainSurface.cpp
)
add_dependencies(MediterraneanClient EmbeddedMeshes)
# Embed artwork in every native client host (including offscreen test hosts).
# Resource include paths make RC independent of the build working directory.
target_sources(MediterraneanClient INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/UiArtwork.rc")
target_include_directories(MediterraneanClient INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui")
add_library(LaunchSupport STATIC launcher/LaunchSupport.cpp)
target_include_directories(LaunchSupport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(LaunchSupport PUBLIC UNICODE _UNICODE NOMINMAX)
target_compile_definitions(LaunchSupport PRIVATE RPG_SAVE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/saves" RPG_PROJECT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(MediterraneanLauncher WIN32 launcher/Main.cpp launcher/Launcher.rc)
target_include_directories(MediterraneanLauncher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/launcher)
target_link_libraries(MediterraneanLauncher PRIVATE LaunchSupport gdiplus shlwapi shell32 user32 gdi32 ole32 uxtheme comctl32)
if(MSVC)
  target_compile_options(LaunchSupport PRIVATE /W4 /EHsc)
  target_compile_options(MediterraneanLauncher PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/EHsc>)
endif()
add_executable(RPGNative WIN32 main.cpp)
target_link_libraries(RPGNative PRIVATE MediterraneanClient LaunchSupport)
add_dependencies(MediterraneanLauncher RPGNative)

target_include_directories(MediterraneanClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(MediterraneanClient PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>")

target_compile_definitions(MediterraneanClient PUBLIC UNICODE _UNICODE NOMINMAX)
target_compile_definitions(MediterraneanClient PRIVATE RPG_SAVE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/saves")

if (MSVC)
  target_compile_options(MediterraneanClient PRIVATE /W4 /EHsc)
endif()

target_link_libraries(MediterraneanClient PUBLIC MediterraneanSimulation d3d11 dxgi dxguid d3dcompiler user32 gdi32 windowscodecs ole32 shell32 psapi xaudio2)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT RPGNative)

include(CTest)
if (BUILD_TESTING)
  add_executable(GltfRigImportTests tests/GltfRigImportTests.cpp)
  target_link_libraries(GltfRigImportTests PRIVATE GltfRigImport)
  target_compile_definitions(GltfRigImportTests PRIVATE RPG_RIG_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hinge-rig.glb")
  target_compile_definitions(GltfRigImportTests PRIVATE RPG_MOTION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hinge-motion.glb")
  target_compile_definitions(GltfRigImportTests PRIVATE RPG_MESH_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hinge-mesh.glb")
  add_test(NAME GltfRigImportTests COMMAND GltfRigImportTests)
  add_test(NAME RigAssetInspectorFixture COMMAND RigAssetInspector "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hinge-rig.glb")
  set_tests_properties(RigAssetInspectorFixture PROPERTIES PASS_REGULAR_EXPRESSION "Validated rig joints: 2")
  add_test(NAME RigAssetInspectorMotion COMMAND RigAssetInspector "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hinge-motion.glb")
  set_tests_properties(RigAssetInspectorMotion PROPERTIES PASS_REGULAR_EXPRESSION "Validated clips: 1")
  add_test(NAME RigAssetInspectorMesh COMMAND RigAssetInspector "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hinge-mesh.glb")
  set_tests_properties(RigAssetInspectorMesh PROPERTIES PASS_REGULAR_EXPRESSION "Validated mesh vertices: 6; triangles: 4")
  add_executable(SkeletonTests tests/SkeletonTests.cpp)
  target_link_libraries(SkeletonTests PRIVATE SkeletonRuntime)
  add_test(NAME SkeletonTests COMMAND SkeletonTests)
  add_executable(BenchmarkStatsTests tests/BenchmarkStatsTests.cpp)
  target_include_directories(BenchmarkStatsTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME BenchmarkStatsTests COMMAND BenchmarkStatsTests)
  add_executable(ClassEquipmentTests tests/ClassEquipmentTests.cpp)
  target_link_libraries(ClassEquipmentTests PRIVATE MediterraneanClient)
  add_test(NAME ClassEquipmentTests COMMAND ClassEquipmentTests)
  set_tests_properties(ClassEquipmentTests PROPERTIES TIMEOUT 45)
  add_executable(ActionClipTests tests/ActionClipTests.cpp)
  target_link_libraries(ActionClipTests PRIVATE MediterraneanClient)
  add_test(NAME ActionClipTests COMMAND ActionClipTests)
  set_tests_properties(ActionClipTests PROPERTIES TIMEOUT 45)
  add_executable(ClassCampaignTests tests/ClassCampaignTests.cpp)
  target_link_libraries(ClassCampaignTests PRIVATE MediterraneanClient)
  target_compile_definitions(ClassCampaignTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME ClassCampaignTests COMMAND ClassCampaignTests)
  set_tests_properties(ClassCampaignTests PROPERTIES TIMEOUT 45)
  add_executable(CharacterSelectionTests tests/CharacterSelectionTests.cpp)
  target_link_libraries(CharacterSelectionTests PRIVATE MediterraneanClient)
  target_compile_definitions(CharacterSelectionTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME CharacterSelectionTests COMMAND CharacterSelectionTests)
  set_tests_properties(CharacterSelectionTests PROPERTIES TIMEOUT 60)
  add_executable(LauncherTests tests/LauncherTests.cpp)
  target_link_libraries(LauncherTests PRIVATE LaunchSupport MediterraneanClient)
  add_dependencies(LauncherTests RPGNative)
  target_compile_definitions(LauncherTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME LauncherTests COMMAND LauncherTests)
  add_test(NAME LauncherUiTests COMMAND MediterraneanLauncher --self-test "${CMAKE_CURRENT_BINARY_DIR}/launcher-ui-check/$<CONFIG>")
  set_tests_properties(LauncherTests LauncherUiTests PROPERTIES TIMEOUT 45)
  add_executable(ObjImportTests tests/ObjImportTests.cpp)
  target_link_libraries(ObjImportTests PRIVATE ObjImport MediterraneanClient)
  target_compile_definitions(ObjImportTests PRIVATE RPG_BLADE_SOURCE="${blade_source}")
  add_test(NAME ObjImportTests COMMAND ObjImportTests)
  add_test(NAME MeshAssetCompilerTests COMMAND ${CMAKE_COMMAND}
    "-DASSET_COMPILER=$<TARGET_FILE:MeshAssetCompiler>"
    "-DASSET_SOURCE=${blade_source}"
    "-DASSET_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/asset-compiler-check/$<CONFIG>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/MeshAssetCompilerTests.cmake")
  add_executable(HandSkinningTests tests/HandSkinningTests.cpp)
  target_link_libraries(HandSkinningTests PRIVATE MediterraneanClient)
  add_test(NAME HandSkinningTests COMMAND HandSkinningTests)
  add_executable(AttachmentLodTests tests/AttachmentLodTests.cpp)
  target_link_libraries(AttachmentLodTests PRIVATE MediterraneanClient)
  add_test(NAME AttachmentLodTests COMMAND AttachmentLodTests)
  add_executable(HandTests tests/HandTests.cpp)
  target_link_libraries(HandTests PRIVATE MediterraneanClient)
  add_test(NAME HandTests COMMAND HandTests)
  add_executable(FaceTests tests/FaceTests.cpp)
  target_link_libraries(FaceTests PRIVATE MediterraneanClient)
  target_compile_definitions(FaceTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME FaceTests COMMAND FaceTests)
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
  add_executable(ClassEffectsTests tests/ClassEffectsTests.cpp)
  target_link_libraries(ClassEffectsTests PRIVATE MediterraneanClient)
  add_test(NAME ClassEffectsTests COMMAND ClassEffectsTests)
  add_executable(FrontierExpansionTests tests/FrontierExpansionTests.cpp)
  target_link_libraries(FrontierExpansionTests PRIVATE MediterraneanClient)
  target_compile_definitions(FrontierExpansionTests PRIVATE RPG_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME FrontierExpansionTests COMMAND FrontierExpansionTests)
  set_tests_properties(FrontierExpansionTests PROPERTIES TIMEOUT 90)
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
  add_executable(GeometryTests tests/GeometryTests.cpp engine/Geometry.cpp engine/Skinning.cpp engine/HandSkinning.cpp game/CharacterPose.cpp)
  target_include_directories(GeometryTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_include_directories(GeometryTests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>")
  add_dependencies(GeometryTests EmbeddedMeshes)
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
  add_executable(ClassCombatTests tests/ClassCombatTests.cpp)
  target_link_libraries(ClassCombatTests PRIVATE MediterraneanSimulation)
  add_test(NAME ClassCombatTests COMMAND ClassCombatTests)
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
