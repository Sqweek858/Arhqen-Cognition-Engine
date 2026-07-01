# ACE-RHI3 + ACE-RHI4

## Scope

```text
ACE-RHI3 = Shader + Root Signature + PSO + actual DrawInstanced
ACE-RHI4 = Offscreen SceneColor/SceneDepth + mesh scene renderer
```

RHI1/RHI2 proved that the DX12 backend can create a device, allocate descriptors, create native resources, upload buffers and clear render targets. RHI3/RHI4 moves the renderer from “GPU does a clear/copy” to “GPU executes real draw calls into offscreen scene targets”.

## RHI3: basic DX12 draw pipeline

RHI3 adds:

```text
- embedded BasicColor HLSL shader source
- D3DCompile for VSMain / PSMain
- empty root signature v1
- input layout:
  POSITION float3
  COLOR0 float4
- graphics PSO creation
- IASetPrimitiveTopology
- SetGraphicsRootSignature
- SetPipelineState
- DrawInstanced
- DrawIndexedInstanced
```

The first shader is intentionally simple:

```text
vertex input:
  float3 position
  float4 color

output:
  SV_Position
  COLOR0
```

No material system yet. No descriptor tables yet. No constant buffer camera yet. This milestone proves real PSO and draw execution without mixing shader/material complexity into the first GPU draw path. Stunning restraint from humanity, finally.

## RHI4: offscreen scene targets

RHI4 adds a helper on `Dx12Device`:

```cpp
bool createOffscreenSceneTargets(Extent2D extent, Texture* color, Texture* depth, std::string* error);
```

It creates:

```text
AceRhi4_OffscreenSceneColor
  BGRA8
  RenderTarget | ShaderResource

AceRhi4_OffscreenSceneDepth
  D32F
  DepthStencil | ShaderResource
```

Both are native DX12 textures with RTV/DSV descriptors.

## Renderer facade path

`AceRenderer` already executes the render graph after RHI1/RHI2. With RHI3/RHI4, a scene with:

```text
- uploaded vertex buffer
- basic color PSO
- offscreen color/depth targets
- MeshBatch
- View
```

can now execute a real DX12 draw pass through:

```text
Renderer
  -> SceneRenderer::buildGraph
  -> RenderGraph::compile
  -> Dx12Device::submit
  -> DrawInstanced / DrawIndexedInstanced
```

## What is still missing

Still not included:

```text
- shader cache / external shader files
- root constants / camera constant buffer
- descriptor tables
- texture sampling
- material system
- mesh upload helper
- offscreen texture composition into the main HWND
- replacement of AQ3D12 CPU/D2D projected viewport
```

## Validation

Run:

```powershell
.\Tools\validate_ace_rhi3_rhi4.ps1
.\Tools\validate_ace_rhi1_rhi2.ps1
.\Tools\validate_ace_rhi0.ps1
.\Scripts\build_release.ps1
```

The new probe validates:

```text
- DX12 runtime exists
- DX12 device initializes
- offscreen scene color/depth targets are created
- vertex buffer uploads to GPU
- basic color PSO is created
- DrawInstanced executes into offscreen target
- shader/PSO/draw GPU stats increment
- Renderer facade renders a MeshBatch into offscreen targets
```

## Next milestone

```text
ACE-RHI5 = main-HWND composition of offscreen DX12 scene texture
ACE-RHI6 = Aquarium/Environment viewport uses GPU mesh grid/blocks instead of D2D projected bridge
ACE-RHI7 = camera constant buffer + world/view/projection transform
```
