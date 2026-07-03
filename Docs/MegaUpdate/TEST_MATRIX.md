# Test Matrix

Populate exact commands/results during M0. Every mini-milestone adds or updates its gate.

| Area | Unit | Integration | Debug build | Release build | Manual/visual | Performance | Status |
|---|---|---|---|---|---|---|---|
| Existing baseline | Aquarium M0-M15 PASS | UI/RHI/bridge probes PASS | PASS | PASS | User telemetry captured | Renderer/UI/present split recorded | Baseline ready; stale probes documented |
| Canonical units | 31 conversion/parser/format checks PASS | MSBuild + CMake integration PASS | PASS | PASS | N/A | No hot-path integration | M1.1 ready |
| Identity/asset paths | 33 GUID/path/sandbox checks PASS | MSBuild + CMake registration PASS | PASS | PASS | N/A | 4096-ID collision smoke PASS | M1.2a ready |
| Versioned persistence | 27 archive/atomic-file checks PASS | MSBuild + CMake registration PASS | PASS | PASS | N/A | Bounded reads/payloads | M1.2b ready |
| Transactions | 21 ordering/group/cancel/budget checks PASS | MSBuild + CMake registration PASS | PASS | PASS | N/A | Bounded history | M1.3 ready |
| Editor commands | 18 registry/context/conflict checks PASS | MSBuild + CMake registration PASS | PASS | PASS | N/A | N/A | M1.4a ready |
| Input routing | 13 z-order/focus/capture/modal checks PASS | MSBuild + CMake registration PASS | PASS | PASS | N/A | N/A | M1.4b ready |
| Global panel resize | 13 edge/bounds/persistence checks PASS | Aquarium shell integrated | PASS | PASS | Manual visual pending | No per-move disk writes | M1.4c ready |
| UI text/style quality | 17 live DirectWrite/cache/style checks PASS | Existing UI/selection/resize probes PASS | PASS | PASS | Manual visual pending | LRU bounded; failed layouts not cached | M1.4d ready |
| Editor workspace layout | 34 model/validation/persistence checks PASS | MSBuild + CMake registration PASS | PASS | PASS | Not exposed yet | 1 MiB/depth/node/tab bounds | M2.1 ready |
| Workspace geometry | 25 solver/hit/resize/tiny-bounds checks PASS | Layout-model regression PASS | PASS | PASS | Not exposed yet | Hidden panes collapse; no negative rects | M2.2 ready |
| Workspace interaction | 31 capture/drag/cancel/commit checks PASS | Model + geometry controller integrated | PASS | PASS | Not exposed yet | One commit on release; none per move | M2.3a ready |
| Visible Engine shell | 12 route/composition/exposure checks PASS | Workspace + single-HWND DX12/D2D integrated | PASS | PASS | User smoke PASS; cleanup applied | No renderer recreation on mode switch | M2.3b ready |
| Premium camera speed | 32 model/routing/popup/reachable-speed checks PASS | AI + Engine viewport control integrated | PASS | PASS | Hidden startup PASS; user feel test pending | Event-time momentum; no frame-rate dependency | M2.4 ready |
| Editor command surface | 20 shell/menu/overlay + 18 registry checks PASS | Shared menu/toolbar/shortcut backend | PASS | PASS | Own-HWND startup capture PASS; editor visual pending | No inactive tools; retained popup overlay | M2.5 ready |
| Content mount / Asset Registry | 24 registry + 60 path/archive checks PASS | Runtime startup mount integrated | PASS | PASS | Empty Content runtime smoke PASS | 250k/64 MiB bounds; indexed lookup | M3.1 ready |
| Live Content watch / deltas | 34 expanded registry/watcher checks PASS | Main-tick debounce + runtime add/remove PASS | PASS | PASS | Content restored empty | 4096 queue; 0.5 s max debounce; overflow rescan | M3.2 ready |
| Asset identity / references | 49 expanded registry/reference checks PASS | Persisted subtree remap + rescan PASS | PASS | PASS | N/A | 65,536 refs/asset; indexed reverse lookup | M3.3a ready |
| Transactional asset operations | 35 operation + registry/transaction regressions PASS | Runtime service/undo-root startup PASS | PASS | PASS | Content remains empty | External exact-byte stash; bounded history | M3.3b ready |
| Content Browser model | 43 navigation/filter/selection/repair checks PASS | Startup + watcher generation sync PASS | PASS | PASS | Hidden empty-root startup PASS | 100,000 assets under 5 s PASS | M3.4 ready |
| Asset operations / scenes | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Materials/shaders | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Rendering/GI | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Mesh/import | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Physics | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Landscape | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Console/profiling | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
