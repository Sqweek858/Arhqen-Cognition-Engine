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
| UI/text/input | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Assets/scenes | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Materials/shaders | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Rendering/GI | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Mesh/import | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Physics | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Landscape | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
| Console/profiling | TBD | TBD | TBD | TBD | TBD | TBD | Not started |
