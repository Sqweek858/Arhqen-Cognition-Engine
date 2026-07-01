# ACE-RHI1 + ACE-RHI2

## Scope

This milestone adds the first real DX12 backend layer to the Arhqen renderer foundation.

```text
ACE-RHI1 = DX12 backend shell
ACE-RHI2 = descriptor heaps + upload buffers
```

The goal is to stop pretending that D2D-projected rectangles are a real engine renderer. That bridge solved flicker, but it is not where the engine should live. The DX12 RHI path now owns real GPU resources and can execute real GPU work.

## What was added

```text
Source/Public/ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h
Source/Private/Renderer/RHI/AceDx12Rhi.cpp
Tools/AceRhi2Dx12GpuProbe.cpp
Tools/validate_ace_rhi1_rhi2.ps1
Docs/ACE_RHI1_RHI2.md
```

## DX12 backend shell

`Dx12Device` implements `IDevice`.

It owns:

```text
- DXGI factory
- hardware adapter selection
- ID3D12Device
- graphics command queue
- command allocator
- graphics command list
- fence + fence event
- frame begin/end lifecycle
- command submission
```

The backend is created through:

```cpp
CreateDx12Device()
CreateDx12DeviceConcrete()
```

The renderer facade now selects the DX12 backend when:

```cpp
DeviceDesc.backend = Backend::Dx12
```

## Descriptor heaps

ACE-RHI2 adds CPU descriptor heap management:

```text
- RTV heap
- DSV heap
- shader-visible CBV/SRV/UAV heap
```

The current allocator is a linear allocator. It is intentionally simple, because the next milestones need stable ownership before fancy recycling. Humanity keeps inventing “clever” allocators and then writing postmortems, so we are not starting there.

## Native resources

The DX12 backend creates native backing resources for RHI handles:

```text
- Buffer -> ID3D12Resource buffer
- Texture -> ID3D12Resource texture
- render target texture -> RTV descriptor
- depth texture -> DSV descriptor
```

RHI handles remain backend-neutral. DX12 native resources are stored internally by handle key.

## Upload path

ACE-RHI2 adds an upload arena:

```text
- committed upload heap
- persistent CPU mapping
- aligned suballocation
- CopyBufferRegion into default GPU buffers
- fence wait for safe smoke-test execution
```

Public helper:

```cpp
Dx12Device::upload(Buffer dst, const void* data, U64 size, std::string* error)
```

This is real GPU copy work, not a fake CPU-side flag flip, because apparently we have standards now.

## GPU clear path

The backend can create a render target texture and clear it on the GPU.

Public helper:

```cpp
Dx12Device::clear(Texture target, Color color, std::string* error)
```

Internally this uses:

```text
- native texture resource
- RTV descriptor
- resource transition
- ClearRenderTargetView
- command queue execute
- fence synchronization
```

Depth clear path is also present for render-pass submission.

## Command submission

`Dx12Device::submit` translates the RHI command list subset:

```text
- barriers
- render pass begin/end
- render target clear
- depth clear
- viewport/scissor
- vertex buffer bind
- index buffer bind
- copy buffer
```

Full PSO/shader translation and actual draw execution are reserved for RHI3/RHI4. Draw commands are validated and carried through the architecture, but full shader/PSO binding is the next layer.

## Renderer facade integration

`AceRenderer` now creates a DX12 backend when requested.

It also executes the render graph instead of only compiling it:

```text
RenderScene
  -> SceneRenderer
  -> RenderGraph compile
  -> RHI execute
  -> DX12 submit
```

For DX12 to do visible draw work, the next milestone needs real PSO translation and offscreen scene color/depth presentation.

## GPU probe

`Tools/AceRhi2Dx12GpuProbe.cpp` validates on Windows:

```text
- D3D12 runtime exists
- hardware adapter exists
- DX12 device initializes
- native default GPU buffer is created
- upload buffer copies vertex data to GPU buffer
- native render target texture is created
- render target clear executes on GPU
- smoke test executes upload + clear
- GPU stats track native buffers/textures/upload bytes/submitted command lists
```

On non-Windows it reports a skip so the sandbox can still run static checks.

## Validation

Run:

```powershell
.\Tools\validate_ace_rhi1_rhi2.ps1
.\Tools\validate_ace_rhi0.ps1
.\Scripts\build_release.ps1
```

## What this still is not

This is not yet the final renderer users see in the Environment viewport.

Still missing:

```text
- shader compiler/cache
- root signatures
- graphics PSO translation
- descriptor tables
- constant buffer binding
- Scene3D mesh draw execution
- offscreen scene color/depth target composition into main HWND
- render thread / frame pacing
```

## Next milestones

```text
ACE-RHI3 = shader/root-signature/PSO translation
ACE-RHI4 = offscreen DX12 scene color/depth target
ACE-RHI5 = single-HWND composition of DX12 texture
ACE-RHI6 = mesh/grid renderer replacing CPU-projected D2D path
```

The important part: the GPU is now actually used by the renderer backend, and the path is ready to stop being an aquarium toy.
