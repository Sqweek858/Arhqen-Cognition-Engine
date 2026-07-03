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
    Source/Private/AquariumUI/AceEnvironmentWorkspace.cpp
    Source/Private/AquariumUI/AceEnvironment3DMode.cpp
    Source/Private/AquariumRender/AceAquariumViewport.cpp
    Source/Private/AquariumRender/AceAquariumSceneAdapter.cpp
    Source/Private/AquariumRender/AceAquariumRenderPrimitive.cpp
    Source/Private/AquariumRender/AceAquariumCamera.cpp
    Source/Private/AquariumRender/AceAquariumEmbeddedDx12Viewport.cpp
    Source/Private/Core/AppConfig.cpp
    Source/Private/Core/Application.cpp
    Source/Private/Core/Assets/AceAssetPath.cpp
    Source/Private/Core/Assets/AceAssetRegistry.cpp
    Source/Private/Core/Assets/AceAssetDirectoryWatcher.cpp
    Source/Private/Core/Assets/AceAssetReferences.cpp
    Source/Private/Core/Identity/AceGuid.cpp
    Source/Private/Core/Logger.cpp
    Source/Private/Core/PathUtils.cpp
    Source/Private/Core/Serialization/AceArchive.cpp
    Source/Private/Core/Serialization/AceAtomicFile.cpp
    Source/Private/Core/Scene/AceSceneWorld.cpp
    Source/Private/Core/RuntimeClock.cpp
    Source/Private/Core/TaskQueue.cpp
    Source/Private/Core/Units/AceUnits.cpp
    Source/Private/Editor/Transactions/AceTransaction.cpp
    Source/Private/Editor/Commands/AceCommandRegistry.cpp
    Source/Private/Editor/Input/AceInputRouter.cpp
    Source/Private/Editor/Workspace/AceEditorWorkspaceLayout.cpp
    Source/Private/Editor/Workspace/AceEditorWorkspaceGeometry.cpp
    Source/Private/Editor/Workspace/AceEditorWorkspaceController.cpp
    Source/Private/Editor/Viewport/AceCameraSpeedModel.cpp
    Source/Private/Editor/Assets/AceAssetOperationService.cpp
    Source/Private/Editor/ContentBrowser/AceContentBrowserModel.cpp
    Source/Private/Editor/ContentBrowser/AceContentBrowserController.cpp
    Source/Private/Editor/Scene/AceSceneEditController.cpp
    Source/Private/Editor/Scene/AceTransformGizmo.cpp
    Source/Private/Editor/Scene/AceScenePicking.cpp
    Source/Private/Renderer/Dx12Renderer.cpp
    Source/Private/Renderer/NativeWindow.cpp
    Source/Private/Renderer/UiDrawList.cpp
    Source/Private/Renderer/Scene3DDrawList.cpp
    Source/Private/Renderer/RHI/AceRhi.cpp
    Source/Private/Renderer/RHI/AceDx12Rhi.cpp
    Source/Private/Renderer/Scene/AceRenderScene.cpp
    Source/Private/Renderer/Scene/AceAquariumGpuViewportRenderer.cpp
    Source/Private/Renderer/Core/AceRenderer.cpp
    Source/Private/Ui/AceShellUi.cpp
    Source/Private/Ui/Slate/AceSlateFramePrimitives.cpp
    Source/Private/Ui/Slate/AceSlateElementList.cpp
    Source/Private/Ui/D2D/AceD2DFlipFrameCompositor.cpp
    Source/Private/Ui/D2D/AceD2DSharedViewportBridgePolicy.cpp
    Source/Private/Ui/Slate/AceSlateRenderer.cpp
    Source/Private/Ui/Slate/AceSlateBatchCache.cpp
    Source/Private/Ui/Slate/AceSlateClipStack.cpp
    Source/Private/Ui/Slate/AceSlateInvalidationPlanner.cpp
    Source/Private/Ui/Slate/AceSlateWindowRendererPipeline.cpp
    Source/Private/Ui/Slate/AceSlateViewportElement.cpp
    Source/Private/Ui/Slate/AceSlateLayerTree.cpp
    Source/Private/Ui/Slate/AceSlatePaintJournal.cpp
    Source/Private/Ui/D2D/AceD2DFrameGraph.cpp
    Source/Private/Ui/D2D/AceD2DFrameDiagnostics.cpp
    Source/Private/Ui/D2D/AceD2DPresentScheduler.cpp
    Source/Private/Ui/D2D/AceD2DViewportBridgeRuntime.cpp
    Source/Private/Ui/D2D/AceD2DViewportCopyScheduler.cpp
    Source/Private/Ui/D2D/AceD2DViewportTextureCache.cpp
    Source/Private/Ui/D2D/AceD2DFrameTransaction.cpp
    Source/Private/Ui/D2D/AceD2DCompositorAudit.cpp
    Source/Private/Ui/D2D/AceD2DViewportFrameLatch.cpp
    Source/Private/Ui/Slate/AceSlateFrameAllocator.cpp
    Source/Private/Ui/Slate/AceSlateFrameTopology.cpp
    Source/Private/Ui/Core/UiAnimation.cpp
    Source/Private/Ui/Core/AceUiInvalidationRoot.cpp
    Source/Private/Ui/Core/AceUiRetainedLayout.cpp
    Source/Private/Ui/Core/AceUiStyleSet.cpp
    Source/Private/Ui/Core/AcePanelResizePolicy.cpp
    Source/Private/Ui/D2D/D2DAutocompletePopup.cpp
    Source/Private/Ui/D2D/D2DBlurRuntime.cpp
    Source/Private/Ui/D2D/D2DCachedEffects.cpp
    Source/Private/Ui/D2D/D2DButton.cpp
    Source/Private/Ui/D2D/D2DClipboard.cpp
    Source/Private/Ui/D2D/D2DCommandHistory.cpp
    Source/Private/Ui/D2D/D2DCommandPalette.cpp
    Source/Private/Ui/D2D/D2DCyberBackgroundField.cpp
    Source/Private/Ui/D2D/D2DCyberEffects.cpp
    Source/Private/Ui/D2D/D2DCyberText.cpp
    Source/Private/Ui/D2D/D2DDiagnosticsOverlay.cpp
    Source/Private/Ui/D2D/D2DDisplayMetrics.cpp
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
    Source/Private/Ui/D2D/D2DTextLayoutFoundation.cpp
    Source/Private/Ui/D2D/D2DDrawCommandBuffer.cpp
    Source/Private/Ui/D2D/D2DUiDebugOverlay.cpp
    Source/Private/Ui/D2D/D2DAquariumTelemetryWidgets.cpp
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
    target_compile_definitions(ArhqenCognitionEngine PRIVATE
        WIN32
        _WINDOWS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
        $<$<CONFIG:Debug>:AM_DEBUG=1>
        $<$<CONFIG:Release>:AM_RELEASE=1;NDEBUG>
    )
    target_link_libraries(ArhqenCognitionEngine PRIVATE d3d12 d3d11 dxgi d3dcompiler dcomp d2d1 dwrite dwmapi)
endif()
