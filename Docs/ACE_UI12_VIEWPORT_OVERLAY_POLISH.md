# ACE-UI12 - Stable Viewport Overlay Layering

## Scop

ACE-UI12 curăță comportamentul UI-ului din Environment 3D după introducerea consolei de log. Ținta este un model plauzibil de tip Slate, nu o clonă UE și nu un rewrite de renderer: viewport-ul este tratat ca un element de paint al ferestrei, iar HUD/telemetry/log console/popup-urile sunt straturi UI cu ordine clară.

## Principiu arhitectural luat din UE/Slate

În Slate, `SViewport` contribuie un viewport draw element în lista ferestrei. Apoi copiii widgetului, cursorul software și overlay-urile se pictează pe layere superioare. Ideea importantă este că scene viewport + UI overlay fac parte dintr-un paint/layer stack controlat, nu dintr-un conflict de z-order între un child/native window și parent UI.

ACE-UI12 păstrează logica, dar pe infrastructura existentă:

1. scena Aquarium se randează prin path-ul GPU existent;
2. când scena deține viewport-ul, DirectComposition zero-copy poate rămâne activ;
3. când UI-ul trebuie să stea peste viewport, scena trece pe parent-composited viewport element;
4. teardown-ul DirectComposition este amânat până după ce frame-ul parent D2D a fost submit-uit;
5. UI-ul de viewport este desenat ca layer stabil: scene, HUD/telemetry, chrome panels, docked log console, popups.

## Layer stack curent

Ordine intenționată:

```text
0. window/background
1. Environment 3D mode background
2. Aquarium scene viewport element
3. viewport HUD / telemetry
4. topbar + left/right panels
5. docked engine log console
6. command palette / diagnostics / modal overlays
```

Asta evită varianta veche în care fiecare bucată de UI se lipea unde apuca și spera că Windows o iubește.

## Fixuri concrete

### Console toggle fără blink agresiv

Backtick (`) nu mai produce toast peste viewport și cere o perioadă scurtă de parent-composited hold. Asta evită ping-pong-ul imediat între DirectComposition și D2D/readback când consola se deschide sau se închide.

### DirectComposition reset deferred

Dacă DirectComposition era activ și un overlay local se deschide, ACE nu îl omoară înainte să existe un frame parent-composited valid. Resetul se face după `EndDraw`, apoi viewport-ul se invalidează pentru frame-ul următor. Practic păstrăm ultimul frame bun până există următorul, ca un UI civilizat.

### Telemetry devine viewport HUD stabil

Telemetry nu mai este desenată mereu bottom-left cu o speranță vagă că acolo va fi liber. Se calculează un rect sigur față de consola docked. Dacă log console ocupă zona de jos, telemetry se mută deasupra ei.

### Input/scroll rămân locale

Console input, PgUp/PgDn și mouse wheel rămân capturate de consola de log cât timp focusul este acolo. Comenzile engine nu ajung la backend.


### ACE-UI12R1: HUD-first parent composition

După testarea în app, toggle-ul consolei era stabil doar până la primul RMB/mouse-look sau camera move. Cauza era DirectComposition: visual-ul scenei se reactiva ca path scene-owned și repicta peste D2D HUD/telemetry/log console. În UI12R1, Environment 3D folosește parent-composited viewport texture cât timp shell-ul are HUD/telemetry peste scenă. DirectComposition zero-copy rămâne o țintă viitoare pentru un mod scene-owned real sau pentru un compositor GPU UI complet, nu pentru UI-ul curent cu overlay-uri active.

Regula practică:

```text
Environment 3D UI visible => parent-composited viewport texture
DirectComposition zero-copy => only when scene owns the viewport and no D2D HUD needs to be above it
```

Asta oprește revenirea la path-ul nativ după RMB/WASD și păstrează stratul de telemetry/log deasupra scenei. Da, FPS-ul poate continua să se comporte ca o glumă proastă, dar măcar UI-ul nu se mai evaporă.

## Non-goals

- nu optimizează FPS-ul;
- nu introduce GPU timestamp queries;
- nu adaugă material system, lighting, terrain sau asset browser;
- nu schimbă arhitectura RHI major;
- nu implementează SlateRHI complet;
- nu ascunde problema de performanță, doar face UI-ul suficient de stabil încât performanța să poată fi diagnosticată ulterior fără mizerii vizuale.

## Validare

Rulează:

```powershell
.\Tools\validate_ace_ui12.ps1
.\Tools\validate_ace_perf0.ps1
```

Validarea ACE-UI12 verifică source-level:

- există layer hold pentru viewport-local overlays;
- DirectComposition reset este deferred după paint;
- log console toggle nu mai aruncă toast;
- telemetry are rect calculat ca HUD stabil și evită log console;
- vechiul child-HWND clipping workaround rămâne scos;
- inputul consolei rămâne local-only.

## ACE-PERF1 follow-up

ACE-PERF1 keeps the UI12 layering rule but makes viewport-only repaint cheaper.
When the dirty rect is limited to the viewport/HUD/log-console layer, ACE repaints
only that retained stack instead of redrawing every side panel. This follows the
same broad principle as Slate viewport painting: the scene viewport is a draw
layer, not a reason to rebuild the entire window chrome.

UI12R1 still forbids child-HWND reactivation over HUD/log UI. PERF1 optimizes the
parent-composited route instead of reopening that particular swamp.

## PERF2 update - GPU-composited viewport overlays

ACE-UI12R1 originally kept the Environment viewport parent-composited while HUD/log UI was visible so the child/native DX12 layer could not cover D2D widgets after RMB/WASD input. ACE-PERF2 supersedes that conservative path for viewport-local overlays: telemetry and the engine log console are baked into the GPU SceneColor texture and presented as `DX12_GPU_COMPOSITED`.

Global parent overlays such as command palette, settings, diagnostics, and shortcut help still force the safe parent-composited fallback. RMB/WASD viewport invalidation must not reactivate an unlayered child HWND over UI; it either stays GPU-composited with baked viewport UI or falls back deliberately.
