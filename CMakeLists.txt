cmake_minimum_required(VERSION 3.20)

project(ArhqenCognitionEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ArhqenCognitionEngine
    Source/Private/App/Main.cpp
    Source/Private/Aquarium/AceAqActions.cpp
    Source/Private/Aquarium/AceAqBody.cpp
    Source/Private/Aquarium/AceAqCounterfactual.cpp
    Source/Private/Aquarium/AceAqDelayedEffects.cpp
    Source/Private/Aquarium/AceAqDynamicWorld.cpp
    Source/Private/Aquarium/AceAqEnvironment.cpp
    Source/Private/Aquarium/AceAqEpisode.cpp
    Source/Private/Aquarium/AceAqExperiment.cpp
    Source/Private/Aquarium/AceAqGrid.cpp
    Source/Private/Aquarium/AceAqJson.cpp
    Source/Private/Aquarium/AceAqMeaningTests.cpp
    Source/Private/Aquarium/AceAqMetrics.cpp
    Source/Private/Aquarium/AceAqObjects.cpp
    Source/Private/Aquarium/AceAqObservation.cpp
    Source/Private/Aquarium/AceAqParity.cpp
    Source/Private/Aquarium/AceAqPlanner.cpp
    Source/Private/Aquarium/AceAqProtoConcept.cpp
    Source/Private/Aquarium/AceAqRandomization.cpp
    Source/Private/Aquarium/AceAqScenario.cpp
    Source/Private/Aquarium/AceAqSelfModel.cpp
    Source/Private/Aquarium/AceAqSensorNoise.cpp
    Source/Private/Aquarium/AceAqSymbol.cpp
    Source/Private/Aquarium/AceAqWorldEvents.cpp
    Source/Private/Aquarium/AceAqWorldModel.cpp
    Source/Private/AquariumUI/AceAquariumLogModel.cpp
    Source/Private/AquariumUI/AceAquariumPanel.cpp
    Source/Private/AquariumUI/AceAquariumRuntimeController.cpp
    Source/Private/AquariumUI/AceAquariumUiSnapshot.cpp
    Source/Private/Core/AppConfig.cpp
    Source/Private/Core/Application.cpp
    Source/Private/Core/Logger.cpp
    Source/Private/Core/PathUtils.cpp
    Source/Private/Core/RuntimeClock.cpp
    Source/Private/Core/TaskQueue.cpp
    Source/Private/Renderer/Dx12Renderer.cpp
    Source/Private/Renderer/NativeWindow.cpp
    Source/Private/Renderer/UiDrawList.cpp
    Source/Private/Ui/AceShellUi.cpp
    Source/Private/Ui/Core/UiAnimation.cpp
    Source/Private/Ui/D2D/D2DAutocompletePopup.cpp
    Source/Private/Ui/D2D/D2DBlurRuntime.cpp
    Source/Private/Ui/D2D/D2DButton.cpp
    Source/Private/Ui/D2D/D2DClipboard.cpp
    Source/Private/Ui/D2D/D2DCommandHistory.cpp
    Source/Private/Ui/D2D/D2DCommandPalette.cpp
    Source/Private/Ui/D2D/D2DCyberBackgroundField.cpp
    Source/Private/Ui/D2D/D2DCyberEffects.cpp
    Source/Private/Ui/D2D/D2DCyberText.cpp
    Source/Private/Ui/D2D/D2DDiagnosticsOverlay.cpp
    Source/Private/Ui/D2D/D2DFocusManager.cpp
    Source/Private/Ui/D2D/D2DGlassEffects.cpp
    Source/Private/Ui/D2D/D2DGraphCanvas.cpp
    Source/Private/Ui/D2D/D2DGraphForceLayout.cpp
    Source/Private/Ui/D2D/D2DGraphInteraction.cpp
    Source/Private/Ui/D2D/D2DGraphLegend.cpp
    Source/Private/Ui/D2D/D2DGraphMinimap.cpp
    Source/Private/Ui/D2D/D2DGraphPreview.cpp
    Source/Private/Ui/D2D/D2DGraphSelectionTrail.cpp
    Source/Private/Ui/D2D/D2DGraphTooltip.cpp
    Source/Private/Ui/D2D/D2DInspectorPanel.cpp
    Source/Private/Ui/D2D/D2DKeyboard.cpp
    Source/Private/Ui/D2D/D2DLayoutOverlay.cpp
    Source/Private/Ui/D2D/D2DLayoutPersistence.cpp
    Source/Private/Ui/D2D/D2DMessageInspector.cpp
    Source/Private/Ui/D2D/D2DMessageLayoutCache.cpp
    Source/Private/Ui/D2D/D2DMessageList.cpp
    Source/Private/Ui/D2D/D2DPropertyGrid.cpp
    Source/Private/Ui/D2D/D2DScrollController.cpp
    Source/Private/Ui/D2D/D2DShortcutHelpOverlay.cpp
    Source/Private/Ui/D2D/D2DSidebar.cpp
    Source/Private/Ui/D2D/D2DSplitter.cpp
    Source/Private/Ui/D2D/D2DStatusBar.cpp
    Source/Private/Ui/D2D/D2DTabStrip.cpp
    Source/Private/Ui/D2D/D2DTextInput.cpp
    Source/Private/Ui/D2D/D2DToastCenter.cpp
    Source/Private/Ui/D2D/D2DToolbar.cpp
    Source/Private/Ui/D2D/D2DWidgetUtils.cpp
    Source/Private/Ui/D2D/D2DWorkspaceLayout.cpp
    Source/Private/Ui/D2D/D2DWorkspacePanel.cpp
    Source/Private/Ui/D2D/DWriteFontEngine.cpp
    Source/Private/Ui/D2D/DWriteTextCache.cpp
    Source/ArhqenCognitionEngine.rc
)

target_include_directories(ArhqenCognitionEngine PRIVATE
    Source/Public
)

if (MSVC)
    target_compile_options(ArhqenCognitionEngine PRIVATE /W4 /permissive-)
else()
    target_compile_options(ArhqenCognitionEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_compile_definitions(ArhqenCognitionEngine PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(ArhqenCognitionEngine PRIVATE d3d12 dxgi d3dcompiler d2d1 dwrite dwmapi)
endif()
