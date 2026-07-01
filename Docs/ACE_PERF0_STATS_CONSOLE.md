# ACE-PERF0 / ACE-LOG0 - Engine Stats Console + Perf/RHI Logging

## Scop

ACE-PERF0 adaugă un set minim de comenzi locale pentru diagnosticarea viewport-ului 3D/DX12 fără să ghicim din ochi de ce FPS-ul se comportă ca un animal speriat. Comenzile rulează din input-ul existent și din command palette, scriu răspunsul ca mesaj Tool/System în message list și append-uiesc un rând în `Build/Logs/ace_engine.log`.

## Comenzi

Toate comenzile merg și cu slash, și fără slash:

- `stat_coords` / `/stat_coords`
- `stat_rhi` / `/stat_rhi`
- `stat_fps` / `/stat_fps`
- `stat_ui` / `/stat_ui`
- `stat_help`, `help`, `/stat_help`, `/help`
- `clear_log` / `/clear_log`

Comenzile locale nu sunt trimise la `submitHandler` / backend. Comenzile necunoscute care încep cu `/` produc warning local și nu sunt trimise la backend.

## Ce măsoară fiecare

### `stat_coords`

Afișează coordonatele și starea de navigație:

- camera position `x/y/z`
- `yaw` și `pitch`
- camera `forward/right/up`
- camera speed
- viewport size
- scenario/planner/step când runtime-ul Aquarium este disponibil
- agent grid position + direction
- active render path: `DX12_ZERO_COPY`, `DX12_READBACK`, `CPU_D2D_FALLBACK`, `CHILD_DX12`, `UNKNOWN`

### `stat_rhi`

Afișează snapshot-ul RHI/viewport:

- active render path
- backend (`DX12` când renderer-ul GPU este creat, altfel `unknown`)
- adapter name (`n/a` în PERF0, pentru că nu există încă API stabil expus din RHI)
- `zeroCopyFrames`, `readbackFrames`, `compositionFrames`, `compositionResizes`
- `targetResizes`, `framesRendered`, `lastPrimitiveCount`, `lastVertexCount`, `lastExtent`
- `uploadBytesAllocated`, `uploadAllocations`
- `nativeBuffers`, `nativeTextures`, `nativePipelines`, `compiledShaders`
- `descriptorAllocations`, `drawCallsExecuted`, `submittedGpuCommandLists`, `completedFenceValue`
- `readbackBytes`, `offscreenSceneTargets`, `wvpConstantsUploaded`

### `stat_fps`

Afișează rolling CPU stats pe ultimele până la 240 frame-uri:

- FPS last/avg/min/max
- frame_ms last/avg/min/max
- `ui_ms`
- `layout_ms`
- `aquarium_build_ms`
- `rhi_render_ms`
- `present_or_composite_ms`
- `fallback_path`

PERF0 folosește doar `std::chrono` pe CPU. Nu introduce GPU timestamp queries și nu blochează frame-ul.

### `stat_ui`

Reutilizează counters existenți din UI/cache:

- text draw count
- ellipsis count
- retained nodes/passes
- draw commands/max
- dirty marks/rects
- styles
- text cache hits/misses
- layout cache hits/misses
- effect cache hits/misses

## Valori care pot fi `n/a`

- `adapter name`: încă nu există API stabil expus din `Dx12Device` către shell.
- `layout_ms`: PERF0 nu instrumentează layout-ul complet ca fază separată.
- `aquarium_build_ms`, `rhi_render_ms`, `present_or_composite_ms`: apar ca `n/a` până există un frame 3D care a trecut prin zona măsurată.
- `backend`: `unknown` înainte ca renderer-ul GPU să fie creat.
- active path: `UNKNOWN` înainte de primul frame 3D.

## Logging

Logul este append-only la comandă și la evenimente locale relevante:

```text
Build/Logs/ace_engine.log
```

### UI log overlay

### Viewport/UI layering note

UE's Slate viewport model does not treat a scene viewport like a random child window floating above the UI. The viewport contributes a draw element to the window draw list, then regular widget children and overlays paint on higher layers. ACE-PERF0R2 mirrors that architecture at the smallest safe scale: when a viewport-local UI layer such as the docked engine log console is visible, ACE disables the DirectComposition visual for the Aquarium viewport and draws the rendered scene as a parent-composited viewport texture. The DX12 scene is still rendered by the GPU; the bridge into the current D2D UI is a cached BGRA bitmap updated from readback until ACE grows a full SlateRHI-style GPU UI renderer.

The legacy child-HWND path is no longer clipped/resized to make room for the console. That old workaround solved z-order visually but caused layout/resize churn and bad pacing. If the scene owns the viewport and no viewport-local overlay is active, DirectComposition zero-copy can still be used. If UI must be over the scene, correctness wins and the parent-composited viewport element path is selected.


Apasă tasta backtick / tilde:

```text
`
```

Aceasta deschide/închide un overlay minimalist peste UI cu ultimele linii din `Build/Logs/ace_engine.log`. Overlay-ul citește tail-ul logului la deschidere și se reîmprospătează după comenzile engine/statistici, nu citește fișierul în fiecare frame. Dacă logul nu există încă, overlay-ul afișează un mesaj local și recomandă rularea unei comenzi `stat_*`.

Format:

```text
[2026-.. ..] [STAT_RHI] path=DX12_ZERO_COPY frames=... zeroCopy=... readback=... verts=...
[2026-.. ..] [STAT_COORDS] camera=(x,y,z) yaw=... pitch=...
[2026-.. ..] [STAT_FPS] fps_avg=... frame_avg_ms=...
```

Dacă folderul `Build/Logs` nu există, este creat automat. Dacă logul nu poate fi scris, aplicația nu crapă; apare warning în UI.

## Non-goals

- Nu optimizează renderer-ul.
- Nu schimbă arhitectura RHI.
- Nu adaugă profiler UI mare, grafic sau overlay full console. Overlay-ul de log este doar tail view text, toggled cu backtick.
- Nu adaugă materiale, lighting, terrain, particles, asset browser sau alte forme moderne de auto-sabotaj.
- Nu adaugă GPU timestamp queries.
- Nu repară resize flicker în acest milestone.

## Validare

Rulează:

```powershell
.\Tools\validate_ace_perf0.ps1
```

Scriptul verifică source-level / CPU-only:

- `stat_coords`, `/stat_coords`, `stat_rhi`, `/stat_rhi`, `stat_fps`
- unknown slash command produce warning local și nu merge la backend
- markerul `/rename` există în continuare
- backtick / `VK_OEM_3` togglează overlay-ul de log
- command palette conține `stat_coords`, `stat_rhi`, `stat_fps`
- docs există
- log helper există
- active render path enum/string există
- formatter-ul RHI include câmpurile cerute
- DirectComposition host can be reset when UI overlays need parent-composited ordering
- the old child-HWND clipping workaround is not used for the log console

## Diagnostic rapid

- **GPU mic + CPU mic + FPS mic** => suspectează wait/present/fence/message loop. Adică mașinăria stă pe loc undeva între frame-uri, minunat.
- **GPU mic + CPU mare** => suspectează CPU/UI/build path: layout, build primitives, D2D composite, logging accidental prea des.
- **GPU mare + FPS mic** => probabil GPU-bound real: draw cost, shader cost, copy/readback/composition cost.

## ACE-LOG0 UI dock update

Press the backtick key (`) to toggle the docked engine log console. The console is anchored to the bottom of the active Environment / 3D viewport instead of floating over the right side of the UI. This keeps the logs visually attached to the render surface being diagnosed.

The dock contains:

- a scrollable tail of `Build/Logs/ace_engine.log`;
- a functional scrollbar for older entries;
- a keyboard input field for local engine commands;
- `Enter` to run the command locally;
- `PgUp` / `PgDn` and mouse wheel to scroll;
- `Esc` or backtick to close.

Commands entered in the dock use the same local dispatcher as the normal input box. `stat_rhi`, `/stat_rhi`, `stat_coords`, `/stat_coords`, `stat_fps`, `/stat_fps`, `stat_ui`, `help`, and `clear_log` are handled locally and are not submitted to the backend. Unknown dock commands produce a warning and are logged with `backend_submit=false`.

## ACE-UI12 viewport overlay polish

Consola de log rămâne docked peste viewport, dar toggling-ul ei este stabilizat:

- backtick nu mai generează toast peste scenă;
- DirectComposition reset este amânat până după submit-ul frame-ului D2D parent;
- un hold scurt păstrează parent-composited viewport path după open/close, ca să nu apară flicker din switching instant;
- telemetry este tratată ca viewport HUD și se mută deasupra consolei când consola ocupă zona de jos.

Validare suplimentară:

```powershell
.\Tools\validate_ace_ui12.ps1
```

## ACE-PERF1 additions to existing commands

`stat_rhi` now also reports mapped dynamic-upload and persistent readback-cache
counters added by ACE-PERF1:

- `mappedUploadBytes`
- `mappedUploadUpdates`
- `readbackBufferReuses`
- `readbackBufferResizes`

`stat_ui` reports viewport repaint mode counters:

- `fast_viewport_paints`
- `full_viewport_paints`

No new `stat_frame` command was added; the existing `stat_fps`, `stat_rhi`, and
`stat_ui` commands remain the diagnostic surface, because apparently even debug
commands can become clutter if left unsupervised.
