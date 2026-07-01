# ACE-AQ3D15 - UE-style Viewport Layering Note

## Scop

ACE-AQ3D15 documentează fixul pentru cazul în care scena 3D DX12 se afișa peste consola/log UI. Problema nu era un simplu z-index: un child HWND sau un DirectComposition visual separat poate sta deasupra parentului D2D, iar parent UI-ul nu îl poate picta peste ca prin magie. Windows, acest muzeu de compromisuri, face exact ce i se cere, doar că nu ce vrem noi.

## Ce am copiat arhitectural din UE/Slate

UE tratează `SViewport` ca widget. Viewport-ul cere renderul scenei, apoi contribuie un element de tip viewport în lista de draw elements. După asta, conținutul copil, cursorul software, debug overlays și restul UI-ului pot fi desenate pe layere mai mari. Renderul final al ferestrei compune scena și UI-ul într-o singură ordine controlată de renderer.

ACE păstrează aceeași idee, fără să copieze cod UE:

1. scena Aquarium se randează în target DX12 offscreen;
2. când nu există UI peste viewport, se poate folosi DirectComposition zero-copy;
3. când există UI peste viewport, DirectComposition este resetat/dezactivat;
4. scena este desenată ca element de viewport în parent D2D, folosind un bitmap BGRA cached;
5. consola/log UI, telemetry și celelalte overlay-uri se desenează după viewport, deci apar deasupra.

## De ce nu mai folosim clip/resize child HWND

Cliparea child HWND-ului ca să facă loc consolei rezolvă doar simptomul vizual. În practică produce rect changes, sync/move churn și pacing prost. Soluția curată este ca UI-ul și viewport-ul să fie în aceeași compoziție logică atunci când UI-ul trebuie să stea peste scenă.

## Limitări

ACE nu are încă un renderer UI complet pe DX12 ca SlateRHI. De aceea calea parent-composited folosește readback BGRA + bitmap D2D cached. Este corectă ca ordering și mai stabilă decât child clipping, dar nu este finalul poveștii pentru performanță. Următorul pas mare, când merită, este UI draw list pe RHI/texture și compoziție GPU reală.

## Validare

`Tools/AcePerf0StatsConsoleProbe.cpp` verifică markerii pentru:

- resetarea DirectComposition host;
- calea `shouldUseDirectCompositionForAquariumViewport`;
- bitmap cached pentru viewport element;
- lipsa vechiului workaround care tăia child HWND-ul sub consola de log.

## ACE-UI12 polish update

ACE-UI12 păstrează modelul de viewport draw element, dar face tranziția mai puțin brutală:

- DirectComposition nu este resetat înainte ca parent D2D să aibă un frame compus valid;
- deschiderea/închiderea consolei cere un scurt parent-composited hold ca să nu apară ping-pong între zero-copy și readback;
- telemetry este mutată într-un viewport HUD layer și își calculează rect-ul ca să evite consola docked;
- backtick toggle nu mai produce toast peste viewport, pentru că overlay-ul este deja feedback vizual.

## ACE-UI12R1 follow-up

The first layering pass still allowed DirectComposition zero-copy to become active again when no docked console was open. That made the native/DComp scene repaint over viewport HUD pieces after RMB mouse-look or camera movement. The follow-up rule is stricter and closer to Slate's useful behavior: while the Environment 3D shell owns HUD/telemetry/console layers, the scene is drawn as a parent-composited viewport texture. Zero-copy is reserved for a future scene-owned mode or a real GPU UI compositor where overlay ordering is explicit.
