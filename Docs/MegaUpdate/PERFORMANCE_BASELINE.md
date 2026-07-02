# Performance Baseline

## Test machine and display

- GPU: NVIDIA GeForce RTX 4060 Laptop GPU, 8 GB dedicated VRAM.
- CPU: Intel Core i9 13th generation laptop CPU.
- Display refresh: 165 Hz.
- Baseline viewport extent in captured telemetry: 1072x900 inside a 1920x991 UI surface.
- Backend: DX12 scene with D2D retained UI through the GPU-sampled D3D11On12/shared-texture bridge.
- VSync config: off; presentation remains display-cadence constrained around 160-165 Hz.

## Captured pre-mega-update telemetry

User capture at 2026-07-02 09:25:

- Full application FPS: last 333.33, average 270.68; average frame 3.69 ms.
- UI metric: 16.66 ms in that sample; RHI render 0.47 ms; present/composite 1.19 ms.
- Raw renderer throughput: 6854.15 FPS.
- Presented cadence: 159.69 FPS.
- Compositor audit: 943/943 frames passed.

User capture at 2026-07-02 09:35 after partial-present experimentation:

- Full application FPS: last 446.67, average 119.64; average frame 8.36 ms.
- UI metric: 4.46 ms.
- Raw renderer throughput: 11303.48 FPS.
- Presented cadence: 165.01 FPS.
- Compositor audit: 741 passed / 18 failed due to unsafe partial viewport frames. This optimization is not a correctness baseline.

## Baseline interpretation

- Scene rendering is not GPU-throughput bound in this tiny scene.
- Raw RHI throughput already exceeds the original 3000 FPS target when measured independently.
- End-to-end editor smoothness is dominated by UI/presentation scheduling, invalidation and interaction cadence rather than primitive rendering.
- Full-frame correctness is the accepted baseline; unsafe partial-present paths cannot be counted as an improvement.
- Future comparisons must report raw renderer throughput, full editor update FPS and presented FPS separately.

## Required future captures

- Release-only controlled idle, camera-look, WASD flight, resize and log-selection cases.
- CPU/GPU/UI/present percentiles and hitch counts over fixed windows.
- Process RAM, dedicated/shared VRAM and allocation counters.
- Exact DPI/scaling, power mode, foreground state and display mode.
