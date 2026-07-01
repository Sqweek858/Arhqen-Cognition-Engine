# ACE-RHI7 + ACE-RHI8

## Scope

```text
ACE-RHI7 = camera constant buffer / WVP shader / real 3D grid-block-agent mesh / depth-tested scene
ACE-RHI8 = DirectComposition shared/composition texture zero-copy path, no CPU readback on the fast path
```

RHI5/RHI6 proved that the Environment viewport can be GPU-rendered and composed through the main HWND without reviving the child HWND flicker path. It still used a GPU-to-CPU readback bridge. RHI7/RHI8 turns that bridge into a real 3D camera path and adds a zero-copy DirectComposition presenter.

## RHI7: WVP shader path

The RHI command stream now supports a WVP constants command:

```cpp
CommandList::wvp(const std::array<float, 16>& value);
```

DX12 translates that into root constants:

```text
D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS
Num32BitValues = 16
register(b0)
```

The BasicColor shader now uses:

```hlsl
cbuffer AceFrame : register(b0)
{
    row_major float4x4 gWvp;
};

output.pos = mul(float4(input.pos, 1.0f), gWvp);
```

This means the viewport geometry is no longer pre-baked into clip-space. The camera supplies the transform, because apparently 3D renderers prefer not being glorified spreadsheet charts.

## RHI7: real 3D Aquarium mesh

`AceAquariumGpuViewportRenderer` now builds actual 3D geometry:

```text
- grid -> thin geometry on the X/Z plane
- tiles -> flat quads
- blocks -> cuboids with shaded faces
- agent -> simple 3D body/glow blocks
- direction arrow -> 3D strip + endpoint marker
- highlights/debug markers -> depth-tested geometry
```

The scene pass is now:

```text
AceRhi7_AquariumDepthTested3DPass
  SceneColor BGRA8
  SceneDepth D32F
  WVP root constants
  DrawInstanced
```

The shell feeds the real `AceAquariumRealCamera::ViewProjectionMatrix(aspect)` into the GPU renderer through `setWorldToClipMatrix`.

## RHI8: DirectComposition zero-copy composition

`Dx12Device` now exposes:

```cpp
bool presentBgra8ToComposition(
    Texture source,
    void* hwnd,
    float left,
    float top,
    Extent2D extent,
    std::string* error);
```

The Windows implementation owns:

```text
- IDCompositionDevice
- IDCompositionTarget
- IDCompositionVisual
- IDCompositionVisual viewport clip
- IDXGISwapChain3 composition swapchain
```

The fast path is:

```text
DX12 SceneColor
  -> GPU CopyTextureRegion to composition swapchain backbuffer
  -> IDXGISwapChain::Present
  -> IDCompositionDevice::Commit
  -> main HWND visual
```

No CPU readback is used on the zero-copy fast path.

The old readback path still exists as a fallback:

```text
DX12 SceneColor -> readback heap -> D2D bitmap
```

That fallback is intentionally kept so a DirectComposition failure does not turn the viewport into a modern art installation called "black rectangle."

## Shell integration

`AceShellUi::renderAquariumSlateCompositeViewport` now attempts:

```text
1. Build Aquarium primitives
2. Compute real camera WVP
3. Render depth-tested GPU 3D scene
4. Present through DirectComposition zero-copy into the main HWND
5. Fall back to readback bitmap if zero-copy fails
6. Fall back to old CPU/D2D bridge if GPU path fails completely
```

The main rule remains:

```text
no child HWND in the main Environment viewport path
```

## Why this is different from RHI5/RHI6

RHI5/RHI6:

```text
GPU rendered 2D/orthographic-ish mesh
GPU -> CPU readback
D2D bitmap composition
```

RHI7/RHI8:

```text
GPU renders real 3D world geometry with camera WVP/depth
GPU copies SceneColor directly to a composition swapchain
DirectComposition displays it in the main HWND
readback only as fallback
```

## Validation

Run:

```powershell
.\Tools\validate_ace_rhi7_rhi8.ps1
.\Tools\validate_ace_rhi5_rhi6.ps1
.\Tools\validate_ace_rhi3_rhi4.ps1
.\Tools\validate_ace_rhi1_rhi2.ps1
.\Tools\validate_ace_rhi0.ps1
.\Scripts\build_release.ps1
```

The new probe creates a hidden HWND on Windows and validates:

```text
- WVP command path compiles
- DX12/DirectComposition fast path initializes
- Aquarium GPU viewport renders 3D mesh geometry
- DirectComposition zero-copy presentation succeeds
- zeroCopyFrames stats increment
```

## Next milestone

Recommended next work:

```text
ACE-RHI9 = render-thread / fixed frame pacing / viewport invalidation timer
ACE-RHI10 = material shader cache + texture sampling
ACE-RHI11 = shared camera constant ring buffer + per-object transforms
```
