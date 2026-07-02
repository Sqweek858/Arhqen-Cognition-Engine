# Arhqen Editor — Mega Update Brainstorm

Acest document este o specificație de lucru pentru promptul final. Nu autorizează implementarea înainte de finalizarea brainstormului.

## 1. AI details (nume intern) — panouri și cameră

"AI details" este doar numele intern folosit în brainstorm pentru această fereastră. Nu trebuie afișat în produs și nu implică redenumirea vreunui element existent.

### 1.1 Sistem global de resize direct din margine

Status: cerință confirmată.

- Se elimină butoanele/mânerele mici de resize aflate în colțul inferior exterior al panourilor (cele indicate în imaginea 2).
- Resize-ul se face direct prin drag pe marginea panoului/widgetului.
- Pentru panoul din stânga: marginea interioară verticală permite redimensionarea lățimii.
- Pentru panoul din dreapta: marginea interioară verticală permite redimensionarea lățimii.
- Marginea inferioară permite redimensionarea înălțimii acolo unde panoul/widgetul are înălțime ajustabilă.
- Colțul rezultat dintre marginea laterală și cea inferioară poate permite resize pe ambele axe, fără un buton vizibil separat.
- Zona interactivă a marginii trebuie să fie suficient de ușor de prins, chiar dacă separatorul vizual rămâne subțire.
- Cursorul se schimbă contextual (`resize horizontal`, `resize vertical`, respectiv diagonal în colț).
- Nu există o limită maximă artificială precum marginea/oprirea indicată în imaginea 3: utilizatorul poate extinde panoul până la limita fizică disponibilă a ferestrei/layoutului.
- Rămâne doar o dimensiune minimă practică, necesară pentru ca panoul să nu ajungă negativ sau imposibil de recuperat. Valoarea exactă se stabilește în specificația finală.
- Resize-ul trebuie să fie continuu și fluid, fără salturi, butoane intermediare sau recreări inutile ale rendererului.
- Dimensiunile finale vor fi salvate în layout și restaurate la următoarea pornire.
- Acesta este un comportament global al sistemului de panouri: orice panou/widget din aceeași categorie, existent sau adăugat ulterior, primește automat resize din margini. Nu se implementează punctual doar pentru panourile din această fereastră.
- Sistemul trebuie centralizat într-o componentă/politică reutilizabilă de panel resize, cu hit-testing, captură mouse, cursoare și persistență comune.

### 1.2 Mouse wheel în viewport — camera speed cu răspuns dinamic

Status: cerință confirmată.

- Când cursorul este deasupra viewportului DX12, rotița mouse-ului controlează viteza camerei.
- Domeniul permis pentru camera speed este `0.0001`–`100000`.
- Schimbarea nu este un increment liniar fix.
- Scroll rapid în sus produce o creștere pozitivă mult mai rapidă a vitezei.
- Scroll lent în sus produce o creștere fină, controlabilă.
- Scroll-ul în jos aplică același principiu în sens invers, reducând viteza.
- Răspunsul trebuie să aibă o curbă lină, cu senzație de arc/accelerație: impulsurile apropiate în timp acumulează momentum, iar impulsurile rare oferă ajustări precise.
- Calculul trebuie să fie independent de FPS și să folosească timpul dintre impulsurile rotiței, nu numărul de cadre.
- Viteza trebuie tratată pe scară logaritmică/multiplicativă pentru ca întregul domeniu `0.0001`–`100000` să rămână utilizabil; valoarea este clamp-uită strict la capete.
- Nu trebuie să existe salturi necontrolate, NaN, overflow sau blocare la zero.
- Scroll-ul controlează camera speed numai când viewportul este ținta corectă; widgeturile scrollabile și overlay-urile aflate deasupra viewportului își păstrează propriul scroll.
- Feedbackul schimbării trebuie să fie imediat și discret (de exemplu o valoare temporară în viewport/popup), fără dialog modal.

### 1.3 Buton Camera și popup pentru valoare directă

Status: cerință confirmată; poziția exactă și aspectul se stabilesc în designul final.

- Se adaugă în UI-ul viewportului un buton cu icon de cameră.
- Click pe buton deschide un popup mic, ancorat de buton, fără fereastră modală mare.
- Popup-ul permite citirea și introducerea directă a valorii camera speed.
- Acceptă valori între `0.0001` și `100000`, cu validare și clamp controlat.
- Trebuie să permită atât valori zecimale foarte mici, cât și valori mari fără pierderea inutilă a preciziei în afișare/editare.
- Enter confirmă, Escape închide fără schimbarea neconfirmată, iar click în exterior închide popup-ul conform comportamentului normal al unui popover.
- Valoarea editată și cea modificată prin scroll reprezintă aceeași proprietate și rămân perfect sincronizate.

### 1.4 Scope închis pentru fereastra AI details

- În afara sistemului global de resize și a controalelor camera speed descrise mai sus, restul ferestrei rămâne neschimbat.

## 2. Engine — fereastra principală a editorului

Status: direcție și scope confirmate.

Butonul `Engine` din fereastra AI details deschide un editor complet în aceeași fereastră principală. Aspectul, organizarea și workflow-ul sunt puternic inspirate din Unreal Editor, dar nu se copiază subsisteme inexistente și nu se afișează controale fără funcționalitate reală.

### 2.1 Intrare, ieșire și stare

- Se adaugă butonul `Engine` în fereastra AI details.
- Editorul pornește în aceeași fereastră top-level; nu se deschide o aplicație separată.
- Utilizatorul poate reveni la interfața AI fără pierderea scenei, selecției sau stării editorului.
- Editorul are propriul layout persistent, separat de layoutul ferestrei AI details.
- Comutarea între moduri nu recreează inutil rendererul, device-ul sau asseturile.

### 2.2 Structura layoutului principal

- Bară de meniu în partea de sus.
- Toolbar funcțional sub meniu.
- Viewport DX12 central.
- `Hierarchy / World Outliner` în dreapta-sus.
- `Details` în dreapta-jos.
- `Content Browser` sub forma unui drawer inferior.
- `Ctrl+Space` deschide și închide Content Browser-ul.
- Toate panourile participă la sistemul global de resize direct din margini.
- Panourile pot fi afișate, ascunse și redimensionate, iar configurația este salvată.
- Docking/tabbing avansat va fi definit separat; nu se presupune automat o clonă completă a frameworkului de docking UE.

### 2.3 Meniuri — numai comenzi funcționale

`File`:

- New Scene.
- Open Scene.
- Save.
- Save As.
- Import, numai pentru formatele implementate.

`Edit`:

- Undo.
- Redo.
- Duplicate.
- Delete.
- Rename.

`Window`:

- Show/Hide Hierarchy / World Outliner.
- Show/Hide Details.
- Show/Hide Content Browser.
- Reset Layout.

`Tools`:

- Material Editor numai după implementarea sa reală.
- Landscape Mode numai după implementarea sa reală.
- Alte unelte apar numai când au backend funcțional.

`Build`:

- Compile Shaders/Materials.
- Rebuild pentru sistemele implementate care au nevoie reală de build.

`Help`:

- Informații despre editor.
- Lista shortcuturilor editorului.

Nu apar `Platforms`, Zen Server, Revision Control sau alte opțiuni UE fără subsistem ACE echivalent.

### 2.4 Toolbar și viewport

- Moduri Select, Move, Rotate și Scale.
- Gizmo complet și interactiv pentru fiecare transformare.
- World/Local coordinate space.
- Translation snapping.
- Rotation snapping.
- Scale snapping.
- Perspective, Top, Bottom, Front, Back, Left și Right views.
- Lit, Unlit și Wireframe numai dacă toate modurile sunt randate real.
- Grid on/off și configurarea dimensiunii gridului.
- Focus Selected.
- Frame All.
- Camera speed cu scroll dinamic și popup-ul Camera definit la secțiunea 1.
- Add Object.
- Play, Pause și Stop numai dacă sunt conectate la simularea reală.
- Selecție prin click.
- Multi-select.
- Box selection.
- Highlight/outline pentru obiectele selectate.
- Drag-and-drop de asseturi din Content Browser în scenă.
- Toolbar-ul nu conține acțiuni inactive sau decorative.

### 2.5 Hierarchy / World Outliner

Outliner-ul reflectă complet și exclusiv scena reală:

- Static meshes și mesh components.
- Lumini.
- Camere.
- Landscape.
- Volume și alte tipuri numai când sunt implementate.
- Foldere editoriale.
- Relații parent-child.
- Componentele deținute de obiecte.

Funcții obligatorii:

- Search.
- Filtre după tip.
- Selectare sincronizată bidirecțional cu viewportul.
- Multi-select.
- Rename.
- Duplicate.
- Delete.
- Drag-and-drop pentru reparenting.
- Drag-and-drop pentru mutarea în foldere.
- Create Folder.
- Visibility toggle.
- Editor lock.
- Expand/collapse.
- Context menu relevant tipului selectat.
- Contor pentru numărul total de obiecte și numărul selectat.
- Icon și type label distinct pentru fiecare clasă suportată.
- Niciun mesh sau obiect real din scenă nu poate lipsi din hierarchy.

### 2.6 Details Panel

Details este generat din proprietățile reale ale selecției și oferă cât mai multe informații/editări utile, fără proprietăți UE fictive.

- Nume.
- Tip.
- ID stabil.
- Cale asset, unde este relevantă.
- Transform: Location, Rotation și Scale.
- Editare numerică directă.
- Numeric drag/scrubbing.
- Reset individual și reset total.
- Uniform scale lock.
- Mobility.
- Visibility.
- Editor lock.
- Parent și componente.
- Mesh asset și statistici mesh.
- Material slots și materialele atribuite.
- LOD-uri și pragurile lor.
- Rendering: visibility, shadows, culling și proprietățile suportate.
- Collision.
- Physics.
- Lighting pentru obiectele relevante.
- Camera settings pentru camere.
- Landscape settings pentru landscape.
- Tags și metadata.
- Categorii pliabile.
- Search și filtre.
- Multi-edit pentru proprietățile comune selecției multiple.
- Orice modificare participă la Undo/Redo.
- Valorile editate modifică imediat obiectul real și persistă în scenă.

### 2.7 Content root izolat

Content Browser-ul este un sandbox pentru asseturile utilizatorului și nu expune structura internă a engine-ului.

- Director dedicat propus: `ArhqenCognitionEngine/Content/`.
- Directorul pornește gol.
- Nu afișează `Source`, `Engine`, `Build`, `Docs`, configuri, cod, shadere interne sau alte fișiere tehnice ale engine-ului.
- Navigarea nu poate ieși din content root.
- În el apar numai folderele și asseturile create sau importate de utilizator.
- Fișierele metadata/cache generate de editor nu poluează vizual Content Browser-ul.

### 2.8 Content Browser

- Arbore de foldere în stânga, similar Windows Explorer/UE Content Browser.
- Grid și list view pentru asseturi în dreapta.
- Breadcrumbs.
- Back, Forward și Up.
- Search.
- Filtre după tip.
- Thumbnails și iconuri după tipul assetului.
- Selecție simplă și multiplă.
- Rename.
- Duplicate.
- Move.
- Delete cu confirmare și verificarea referințelor.
- Drag-and-drop între foldere.
- Drag-and-drop în viewport pentru tipurile compatibile.
- Metadata proprie editorului fără alterarea inutilă a sursei importate.
- Content Browser poate fi redimensionat prin sistemul global de panel resize.
- Shortcut global în editor: `Ctrl+Space` pentru toggle drawer.

### 2.9 Create/Add în Content Browser

Click stânga pe butonul `+ Add` deschide meniul de creare. Click dreapta în spațiul liber poate deschide același meniu contextual. Un click stânga arbitrar în spațiul gol rămâne disponibil pentru deselectare și nu deschide meniul automat, dacă utilizatorul nu confirmă ulterior contrariul.

Opțiuni, afișate numai când tipul este implementat:

- New Folder.
- Material.
- Material Instance.
- Scene/Level.
- Import FBX.
- Import Texture.
- Import Mesh pentru formatele implementate.
- Alte asset types numai după existența editorului și runtime-ului lor real.

### 2.10 Regula anti-placeholder

- Niciun buton mort.
- Niciun meniu placeholder.
- Niciun asset type care nu poate fi creat, salvat, redeschis și utilizat.
- Niciun checkbox fără efect real.
- Nicio statistică inventată.
- Nicio proprietate Unreal care nu are echivalent real în ACE.
- Orice feature indisponibil este omis complet din UI, nu afișat disabled pentru decor.
- Interfața poate imita foarte fidel organizarea și ergonomia UE, dar conținutul reprezintă exclusiv capabilități ACE funcționale.

### 2.11 Excluderi explicite pentru acest mega update

- Nu se implementează sistemul de animații/skeletal animation în acest update.
- Nu se implementează un mini-Niagara/particle editor în acest update.
- Nu se afișează controale sau asset types pentru aceste sisteme.
- Aceste subsisteme sunt rezervate unui update ulterior, după stabilizarea editorului, materialelor, shader compilerului și fizicii.

## 3. Asset Editor Host, naming, import FBX și unități

Status: direcție confirmată.

### 3.1 Fereastra comună pentru asset editors

- Dublu-click pe un asset editabil din Content Browser îl deschide într-o fereastră secundară nativă ACE.
- Fereastra este legată de aceeași aplicație și același lifecycle ACE; nu este un proces sau o instanță separată a engine-ului.
- Este grupată sub aceeași identitate ACE în taskbar, dar poate fi mutată, minimizată, maximizată și restaurată independent de fereastra principală.
- Există un singur `Asset Editor Host` reutilizabil.
- Fiecare asset se deschide într-un tab, similar taburilor Chrome/Unreal Asset Editors.
- Deschiderea unui alt material, mesh sau asset compatibil creează un tab nou în același host.
- Dacă assetul este deja deschis, se activează tabul existent, fără duplicare.
- Dacă host-ul este minimizat, dublu-click pe asset îl restaurează și focalizează tabul potrivit.
- Taburile au icon de tip, nume, dirty indicator și close.
- Închiderea unui tab modificat oferă Save/Discard/Cancel.
- Sunt suportate Save, Save All, `Ctrl+S`, `Ctrl+Shift+S` și `Ctrl+W` unde au sens.
- Starea taburilor și geometria ferestrei pot fi restaurate controlat între sesiuni.

Editoare pe tip, disponibile numai după implementare reală:

- Material → Material Graph Editor.
- Static Mesh → Mesh Editor.
- Texture → Texture Viewer/Settings.
- Material Function → Material Function Graph.
- Alte tipuri primesc tab numai după existența editorului funcțional corespunzător.

### 3.2 Naming și rename

- La crearea unui asset, Content Browser intră imediat în inline rename și permite introducerea numelui.
- Numele este validat pentru caractere, cale rezervată, lungime și coliziuni.
- Numele duplicat produce feedback clar și nu suprascrie silențios.
- `F2` redenumește assetul selectat în Content Browser.
- `F2` redenumește obiectul/folderul selectat în Hierarchy, când focusul aparține acelui panou.
- Rename-ul unui asset actualizează registrul, metadata și toate referințele ACE, nu doar numele fișierului.
- Operația participă la Undo/Redo unde poate fi făcută tranzacțional și sigur.

### 3.3 Popup real pentru import FBX

- Alegerea unui FBX deschide un popup de import înainte de crearea asseturilor.
- Popup-ul prezintă sursa, conținutul detectat, opțiunile și un rezumat al rezultatului.
- Detectează unitatea declarată în FBX.
- Afișează unitatea proiectului, conversion factor și import scale final.
- Permite configurarea Up Axis și Forward Axis.
- Aplică explicit conversia de handedness/coordonate.
- Permite bake/apply transform.
- Oferă import normals/tangents sau generarea lor, numai dacă ambele căi sunt implementate.
- Triangulation.
- Weld vertices cu toleranță controlată.
- Combine meshes sau import separat.
- Material slots.
- Import LOD-uri.
- Collision import/generation numai după existența backendului real.
- Preview/summary cu mesh count, vertices, triangles, materials, LOD-uri, warnings și errors.
- Preseturi de import salvabile opțional.
- Nicio opțiune afișată nu este ignorată de importer.

### 3.4 Sistem unitar de măsură

- Engine-ul utilizează o singură unitate internă canonică; alegerea finală va fi confirmată după auditarea scării existente și a backendului de fizică.
- Recomandarea curentă este metrul intern, cu afișare configurabilă.
- Unități metrice obligatorii: `mm`, `cm`, `dm`, `m`, `km`.
- Câmpurile numerice acceptă sufixe, de exemplu `8mm`, `40cm`, `2.5m`.
- Valorile fără sufix folosesc unitatea implicită a proiectului/câmpului.
- Schimbarea unității de afișare nu redimensionează obiectele existente.
- Details, transform gizmos, grid, snapping, camera, landscape, physics, FBX import și export folosesc aceeași bibliotecă de conversie.
- Conversiile evită pierderea inutilă de precizie și au formatare adaptivă.
- Scale import și unit conversion sunt operații distincte, vizibile și predictibile.
- Unitățile non-metrice pot fi adăugate ulterior numai dacă există nevoie reală.

## 4. Material Graph Editor

Status: arhitectură și set inițial de capabilități confirmate.

Implementarea trebuie precedată de studierea atentă a arhitecturii Material Editor/Material Compiler din UE source. Se adaptează conceptele la ACE; nu se copiază orbește cod sau subsisteme incompatibile.

### 4.1 Layout

- Preview în stânga-sus.
- Details pentru material/nodul selectat în stânga-jos.
- Graph canvas central.
- Toolbar sus.
- Compile status, warnings și errors jos.
- Materialele și Material Functions coexistă ca taburi în Asset Editor Host.
- Selecția unui nod schimbă Details la proprietățile reale ale nodului.

### 4.2 Preview

- Sphere, Cube, Plane/Wall și Cylinder.
- Mesh custom ales din Content Browser.
- Orbit, pan și zoom.
- Environment și lumini configurabile.
- Grid/background.
- Realtime preview on/off.
- Reset camera.
- Preview material complet și, unde tipul permite, preview pentru nodul selectat.
- Preview-ul folosește shaderul real compilat, nu o aproximație 2D.

### 4.3 Material Output

- Creat automat la crearea materialului.
- Unic și imposibil de șters.
- Pinii sunt condiționați de material domain, blend mode, shading model și suportul real al pipeline-ului.
- Pachet PBR țintă: Base Color, Metallic, Specular, Roughness, Anisotropy, Emissive Color, Opacity, Opacity Mask, Normal, Tangent, Ambient Occlusion, Cavity, World Position Offset, Displacement, Subsurface Color, Clear Coat, Clear Coat Roughness, Refraction, Pixel Depth Offset și Material Attributes.
- Pinii fără backend funcțional sunt omiși până la implementare.
- `Use Material Attributes` comută output-ul la intrarea structurată Mater…2094 tokens truncated…ance LUT.
46. Sky Atmosphere multi-scattering LUT.
47. Sky View LUT.
48. Aerial Perspective.
49. Height Fog.
50. Volumetric Fog injection.
51. Volumetric Fog integration.
52. Volumetric Clouds numai dacă sistemul complet rămâne în buget; altfel este omis integral.

### 5.5 Post-process passes

53. Auto-exposure histogram.
54. Auto-exposure adaptation.
55. Bloom downsample.
56. Bloom upsample/composite.
57. ACES tone mapping.
58. Color grading/LUT.
59. TAA.
60. FXAA.
61. Motion Blur.
62. Depth of Field.
63. Vignette.
64. Chromatic Aberration.
65. Film Grain.
66. Sharpen/CAS.
67. Spatial Upscale.
68. Screen Percentage resolve.
69. Gamma/output transform.
70. Debug visualization composite.

### 5.6 ACE Global Illumination

Sistemul se numește `ACE GI`. Este inspirat arhitectural de ideile Lumen studiate în UE source, dar este adaptat și redus pentru ACE; nu se copiază textual implementarea și nu se pretinde paritate completă cu Lumen.

- Screen traces pentru informația vizibilă și ieftină.
- DXR hardware tracing pe hardware compatibil, inclusiv RTX 4060.
- BLAS/TLAS și lifecycle corect pentru geometrie statică/dinamică.
- Surface Cache pentru material/radiance representation.
- Screen Probes pentru final gather.
- Radiance Cache pentru reutilizare spațială.
- Temporal reprojection.
- History validation/rejection la disocclusion, camera cuts și mișcare.
- Spatial denoising.
- Diffuse indirect lighting.
- Glossy reflections.
- Skylight și lumini dinamice.
- Emissive materials contribuie la GI.
- Update incremental pentru obiecte și lumini în mișcare.
- Debug visualization pentru fiecare etapă.

Fallback fără DXR:

- SSGI.
- Screen-space reflections.
- Probe/radiance cache.
- Software tracing prin mesh/global distance fields poate fi adăugat numai ca implementare completă ulterioară.

### 5.7 Render Graph

- Sistem conceptual apropiat de UE RDG, adaptat ACE.
- Declararea pass-urilor și a dependențelor read/write.
- Resource lifetime automat.
- Resource-state transitions și barriers corecte.
- Transient textures/buffers și aliasing sigur.
- Pass culling.
- Async compute unde dependențele permit.
- GPU markers.
- CPU/GPU timings.
- Graph dump și inspector.
- Validare în debug pentru hazards și resurse neinițializate.

### 5.8 Shader Compiler Service

- DXC și Shader Model 6.x.
- Compile jobs asincrone.
- Include resolver virtual.
- Sandbox pentru cod user: `Content/Shaders/`.
- Reflection.
- Permutation domains controlate.
- Hash determinist.
- Disk/derived-data cache.
- Dependency tracking.
- Hot reload.
- Compile workers.
- Diagnostics mapate la asset, nod și linie.
- Invalidarea controlată a PSO-urilor dependente.
- Ultimul shader valid rămâne activ la compile failure.

### 5.9 Pipeline State Cache

- PSO descriptors deterministe.
- Cache în memorie și pe disk.
- Prewarming.
- Fallback PSO sigur și explicit.
- Fără PSO compilation necontrolată în frame.
- Cache hit/miss/stall reporting.

### 5.10 Scene rendering foundation

- Linear HDR pipeline.
- GBuffer documentat și versionat.
- Reversed-Z.
- Depth pyramid/Hi-Z.
- GPU frustum culling.
- Occlusion culling.
- Indirect draws.
- Instancing.
- LOD selection.
- Descriptor management robust/bindless unde hardware-ul și designul permit.
- Material parameter buffers.
- Per-view și per-object constants.
- Frame History Manager pentru TAA/GI/reflections.
- GPU profiler.

### 5.11 Editor — Rendering Settings

- Renderer path și feature level.
- Forward/Deferred numai dacă ambele sunt implementate real.
- HDR.
- Anti-aliasing.
- Screen Percentage.
- Shadow, GI, reflection și volumetric quality.
- Texture/anisotropic filtering.
- Debug/profiling controls.
- Setările sunt serializate la nivel de proiect/scenă conform scope-ului lor.

### 5.12 Post Process Volume și Profile

Post Process Volume este obiect real în Hierarchy și Details:

- Infinite Extent.
- Priority.
- Blend Radius și Blend Weight.
- Exposure.
- Bloom.
- Tone Mapping.
- Color grading și LUT.
- White balance.
- Saturation, contrast, gamma și gain.
- Vignette, chromatic aberration și film grain.
- Motion Blur și Depth of Field.
- GI/reflection overrides.
- Anti-aliasing și sharpening.
- Override checkbox per proprietate pentru volume blending.

`Post Process Profile` este asset reutilizabil în Content Browser și Asset Editor Host:

- asignabil volumelor și camerelor;
- duplicabil/editabil;
- preview before/after;
- serializare completă.

### 5.13 Shader Compiler Dashboard

- Jobs active și coadă.
- Success/failure și durată.
- Cache hit/miss.
- Shader permutations.
- PSO dependencies.
- Recompile Selected/All.
- Erorile deschid materialul, nodul sau HLSL-ul relevant.

### 5.14 Viewport debug modes

- Base Color, Metallic, Roughness, Specular și Normal.
- AO, Emissive, Depth și Motion Vectors.
- Overdraw, Shader Complexity și Light Complexity.
- LOD Coloration.
- NaN/Inf detection.
- Direct Lighting, Diffuse Indirect și Reflections.
- Screen Traces, Surface Cache, Radiance Cache și Screen Probes.
- GI history rejection.

### 5.15 Render Graph Inspector

- Lista și ordinea pass-urilor.
- Dependencies.
- CPU/GPU timings.
- Resurse, dimensiuni și formate.
- Barriers și transient memory.
- Render-target preview.
- Export frame report.

### 5.16 Scalability și performanță

- Profile: Off, Low, Medium, High și Cinematic.
- Fiecare feature poate fi măsurat separat.
- Baseline-ul simplu păstrează obiectivul de throughput foarte ridicat.
- Nu se pretinde 3000 FPS la rezoluție nativă cu GI, reflections, volumetrics și post-process complet active; fiecare profil are buget și telemetry explicit.

## 6. Static Mesh Editor și mesh pipeline

Status: design delegat pentru un workflow UE-standard adaptat ACE, fără Nanite placeholder.

- Static Mesh assets se deschid în Asset Editor Host.
- Preview 3D: Lit, Unlit, Wireframe, Normals, Tangents, UV, Collision și LOD coloration.
- Orbit/pan/zoom, camera speed, environment și grid.
- Statistici: vertices, triangles, sections, UV channels, material slots, bounds și memorie.
- Material Slots: add/remove/reorder/rename/assign și section mapping.
- LOD0 plus LOD-uri importate sau generate.
- Screen-size threshold per LOD și auto-compute distances.
- Triangle/vertex reduction target.
- Preserve boundaries, UV seams, normals și material borders.
- Forced LOD preview și comparative stats.
- Normals import/recompute/weighted.
- Tangents import/recompute și MikkTSpace.
- UV viewer pe canal.
- Generate/pack lightmap UV, lightmap channel și resolution.
- Pivot editing, bounds și bake transform.
- Import/reimport FBX cu păstrarea setărilor și material assignments când este posibil.
- Socket system: create, rename, transform, delete și preview.
- Collision visualization.
- Simple collision: box, sphere, capsule și convex.
- Auto convex decomposition.
- Complex-as-simple și simple-as-complex.
- Collision complexity și Physics Material.
- Apply/Save, dirty state, dependencies și Find References.
- Nicio opțiune Nanite sau mesh feature fără implementare reală.

## 7. Physics

Status: design UE-standard adaptat ACE. Recomandare: API ACE propriu peste un backend matur precum Jolt, ales definitiv după audit tehnic/licență/build integration.

### 7.1 Physics World

- Fixed timestep și accumulator independent de FPS.
- Substepping.
- Gravity configurabilă.
- Broad phase și narrow phase.
- Collision layers/masks.
- Sleeping/waking.
- Continuous Collision Detection.
- Determinism settings.
- Debug drawing.
- Async simulation numai cu sincronizare sigură.

### 7.2 Rigid bodies și shapes

- Static, Kinematic și Dynamic.
- Mass/density și center of mass.
- Linear/angular damping și velocity.
- Gravity scale, max velocity și sleep thresholds.
- CCD și axis locks.
- Forces, impulses și torques.
- Box, Sphere, Capsule, Cylinder, Convex Hull, Compound, static Triangle Mesh, Landscape Heightfield și Trigger/Overlap Volume.

### 7.3 Physics Materials, constraints și queries

- Physics Material asset: friction, restitution, density și combine modes.
- Fixed, Hinge, Slider, Ball-and-Socket, Distance, Spring și 6DoF constraints.
- Limits, motors, break force/torque și visualization.
- Raycast, sphere/capsule/box cast și overlaps.
- Query filters.
- Collision/trigger enter, stay și exit.
- Contact points, normals și impulses.

### 7.4 Physics editor workflow

- Simulate Physics, Play, Pause, Step și Stop.
- Reset la starea pre-simulation.
- Keep Simulation Changes explicit.
- Collision/constraint debug overlays.
- Collision Layers editor și Project Physics Settings.
- Proprietăți complete în Details.
- Undo/Redo în afara simulării.
- Cloth, destruction și ragdoll sunt excluse deoarece animațiile sunt excluse.

## 8. Landscape și basic world partition

### 8.1 Landscape creation

- World width/length cu unități.
- Height range și heightmap resolution.
- Section size, sections per component și component count.
- Estimări live pentru quads, vertices, triangles și memorie.
- LOD count/distribution și target triangles per distance band.
- Flat landscape sau import heightmap.
- Scale XY/Z și material inițial.

### 8.2 Basic world partition

- Cell/chunk size.
- Loading radius și unloading hysteresis.
- Maximum loaded cells.
- CPU/GPU memory budget.
- Per-cell landscape data și save.
- Streaming priority și background streaming.
- LOD per cell și seam handling.
- Origin rebasing pentru lumi mari.
- Grid și loaded/unloaded-cell visualization.
- Lock, force load și force unload cell.
- Validare pentru configurații imposibile.
- Estimări live pentru chunks, triangles și memorie.
- Preseturi Small, Medium, Large și Custom.
- Este un streaming grid ACE robust, nu o clonă incompletă a întregului UE World Partition.

### 8.3 Minimum 30 de sculpt tools

1. Sculpt Raise.
2. Sculpt Lower.
3. Smooth.
4. Detail Smooth.
5. Flatten To Target.
6. Flatten To Average.
7. Flatten To Minimum.
8. Flatten To Maximum.
9. Ramp.
10. Terrace.
11. Plateau.
12. Pinch.
13. Inflate.
14. Scrape.
15. Fill Depressions.
16. Relax.
17. Sharpen.
18. Ridge.
19. Valley.
20. Crater.
21. Dune.
22. Canyon.
23. River Channel.
24. Coastline/Beach.
25. Hydraulic Erosion.
26. Thermal Erosion.
27. Wind Erosion.
28. Noise.
29. Voronoi Terrain.
30. Heightmap Stamp.
31. Alpha Stamp.
32. Spline Deform.
33. Retopologize/Redistribute.
34. Mirror.
35. Copy/Paste Region.
36. Visibility/Hole Tool.

Setări comune unde sunt relevante: radius, strength, falloff/curve, flow, spacing, jitter, rotation, scale, pressure, invert, target height, iterations, seed, height/slope restriction, selection/layer mask, symmetry, realtime preview și GPU compute. Undo/Redo este per stroke, nu per sample. Sculpt layers non-destructive se implementează numai dacă pot fi livrate complet.

### 8.4 Landscape Paint

- Landscape material real și Material Graph landscape nodes.
- Weight-blended și non-weight-blended layers.
- Add/remove/rename layer.
- Paint, erase, fill și clear.
- Layer opacity/visibility.
- Per-layer textures/material attributes și Physics Material.
- Slope/height procedural masks.
- Auto material plus corecție manuală.
- Brush alpha și setările comune de sculpt.
- Layer debug visualization și weight normalization.
- Salvare per partition cell.
- Preview identic cu runtime shader.

## 9. Scene, assets și transactions

- Scene format versionat propus `.acescene`.
- GUID stabil pentru obiecte și asseturi.
- Asset Registry.
- Hard/soft references prin ID/cale virtuală.
- Salvare atomică.
- Autosave și crash recovery.
- Schema versions și migrations.
- Missing-reference diagnostics și scene diff.
- Binary payload separat pentru mesh, texture și landscape.
- Fișierele interne rămân ascunse din Content Browser.
- Transaction system central și scoped transactions.
- Transform drag este o singură operație.
- Graph/property/object/asset/landscape/material/import operations participă la Undo/Redo unde reversarea este sigură.
- Istoric și memory budget configurabile.

## 10. Lighting, World Settings și texture pipeline

- Directional, Point, Spot, Rect și Sky Light.
- Sky Atmosphere, Height Fog și Volumetric Fog.
- Unități fizice potrivite tipului: lux, lumens și candelas.
- Color/temperature, range, cones, source dimensions și shadows.
- Cascades, bias, normal bias și contact shadows.
- World gravity, environment, exposure defaults, ACE GI, bounds și origin rebasing.
- Time-of-day control de bază.
- Texture imports: PNG, JPEG, TGA, BMP, HDR, EXR și DDS compatibil.
- sRGB/Linear și presets Color, Normal, Masks, HDR și UI.
- Mipmaps, filtering/sharpening și alpha detection.
- BC1/3/4/5/6H/7 unde sunt suportate.
- Normal-map validation, channel preview, memory stats și reimport.
- Sursa platform-independent este separată de derived GPU data.

## 11. Docking, layouts și shortcuts

- Splitter tree și docking left/right/top/bottom/center.
- Tab stacks, tab drag și panel close/reopen.
- Detach controlat în Asset Editor Host.
- Save Layout, named layouts și Reset Layout.
- Shortcut editor, conflict detection și searchable command palette.
- Persistență separată Main Editor/Asset Editor Host.
- Sistemul global de resize din margini rămâne fundația tuturor panourilor.

## 12. Teste de acceptare

- Debug/Release builds.
- Scene save/reload parity.
- Asset rename/move/reference parity.
- Undo/Redo stress.
- FBX import/reimport.
- Shader compile failure/success/hot reload/cache.
- Material graph serialization și dependency rebuild.
- Mesh LOD/collision.
- Physics determinism, contacts și queries.
- Landscape sculpt/paint/save/streaming.
- Resize/docking/DPI.
- GPU resource lifetime și device recovery.
- Leak checks și performance telemetry per subsystem.
- Automated rendering comparisons și checklist manual pentru utilizator.

## 13. Engine Console, commands și debugging/profiling

### 13.1 Console lifecycle și prezentare

- Consola vizibilă pornește curată la fiecare deschidere și este curățată la închidere.
- Fiecare sesiune a aplicației pornește cu bufferul consolei curat.
- Logurile persistente de crash/error sunt gestionate separat, astfel încât cleanup-ul UI să nu distrugă informația necesară diagnosticului post-crash.
- Fără rânduri goale aleatorii, spacing instabil sau wrapping corupt.
- Randare monospace, virtualized și performantă pentru volume mari.
- Text selection vizual exact, copy corect, double-click word și triple-click line.
- Scroll, drag selection autoscroll și scrollbar au lifecycle corect.
- Severity colors pentru trace/info/warning/error/fatal.
- Filtre pe categorie/severity/text.
- Search, next/previous result.
- Timestamps și source category configurabile.
- Command history, autocomplete și suggestions.
- Comenzile și erorile apar imediat și lizibil pe ecran.
- Comenzile care activează un overlay oferă feedback vizual discret și starea curentă.

### 13.2 `stat_fps` simplificat

Outputul implicit conține maximum câteva valori esențiale:

- FPS current/rolling average.
- Frame time current/average.
- CPU frame ms.
- GPU frame ms.
- UI ms.
- Present/display cadence.

Detaliile extinse se mută în `stat_fps_detail` și/sau într-un dump separat.

### 13.3 `stat_rhi` simplificat

Output implicit scurt:

- Backend și adapter.
- Resolution/present mode.
- CPU/GPU frame time.
- Draw calls și triangles.
- VRAM used/budget.
- Render path și active major features.

Detaliile complete se mută în `stat_rhi_detail`, Render Graph Inspector sau dump JSON/text.

### 13.4 Command set extins

Comenzile sunt înregistrate într-un registry comun, cu help, argument schema, autocomplete și validare. Set minim propus:

- `help`, `help <command>`, `clear`, `history`.
- `stat_fps`, `stat_fps_detail`, `stat_frame`.
- `stat_rhi`, `stat_rhi_detail`, `stat_gpu`, `stat_cpu`, `stat_memory`.
- `stat_draws`, `stat_triangles`, `stat_meshes`, `stat_materials`, `stat_shaders`, `stat_pso`.
- `stat_textures`, `stat_streaming`, `stat_landscape`, `stat_physics`, `stat_gi`.
- `profile_start`, `profile_stop`, `profile_capture`, `profile_dump`.
- `gpu_capture_marker`, `dump_render_graph`, `dump_resources`, `dump_scene`, `dump_asset_registry`.
- `shader_recompile`, `shader_recompile_all`, `shader_cache_stats`, `pso_cache_stats`.
- `rhi_validation`, `renderdoc_capture` numai dacă integrarea există real.
- `view_wireframe`, `view_overdraw`, `view_shader_complexity`, `view_light_complexity`.
- `view_lod`, `view_collision`, `view_normals`, `view_tangents`, `view_uv`, `view_velocity`.
- `view_gbuffer <channel>`, `view_depth`, `view_object_id`.
- `view_gi`, `view_gi_probes`, `view_gi_surface_cache`, `view_gi_history`.
- `tri_validate`, `tri_degenerate`, `tri_backface`, `tri_density`, `tri_winding`.
- `memory_gc`, `memory_report`, `memory_allocations` numai cu implementare sigură.
- `physics_pause`, `physics_step`, `physics_debug`.
- `landscape_cells`, `landscape_lod`, `streaming_freeze`.
- `ui_invalidation`, `ui_batches`, `ui_layout`, `ui_profile`.

### 13.5 Triangle și geometry debugging

- Wireframe overlay.
- Triangle-density heatmap.
- Overdraw heatmap.
- Degenerate triangle detection.
- Backface/winding visualization.
- Non-manifold/open-edge diagnostics unde datele mesh permit.
- Vertex normal și tangent visualization.
- UV seam/stretch/overlap visualization.
- LOD coloration și forced LOD.
- Per-object triangle/vertex counts.
- Selection-only și whole-scene modes.
- Raport exportabil cu asset, section, LOD și problema detectată.

### 13.6 CPU/RAM/GPU/VRAM telemetry

- Process working set, private bytes/commit și allocation counters.
- RAM current/peak și categorii ACE unde allocatorul le poate eticheta.
- CPU process utilization, per-thread utilization și thread names.
- Main/render/worker thread timings.
- GPU adapter utilization și engine utilization unde API-ul Windows oferă date fiabile.
- Dedicated/shared VRAM used, budget, resident și evicted.
- Buffer/texture/render-target allocation counts și bytes.
- Descriptor heap usage.
- Upload/readback bytes și stalls.
- Shader/PSO/cache memory.
- Landscape/physics/material/mesh memory categories.
- Historical graphs și spike markers.

### 13.7 Profiling și optimization tools

- CPU scoped events și timeline.
- GPU timestamp queries și pass timings.
- Frame-time history, histogram, percentiles și 1%/0.1% lows în profilerul detaliat.
- Automatic hitch detection și capture ring buffer înainte/după spike.
- Render Graph pass/resource inspection.
- Draw-call, triangle, material switch și PSO switch counters.
- Resource lifetime și transient-memory visualization.
- Shader instruction/sampler/permutation stats.
- Culling efficiency și LOD distribution.
- Physics, landscape streaming și asset streaming timings.
- UI invalidation/batch/cache profiling.
- Overlay compact configurabil.
- Full profiler window în editor.
- Export text/JSON/CSV pentru analize ulterioare.
- Profiling-ul dezactivat are overhead minim; instrumentarea costisitoare este opt-in.

