$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path (Join-Path $Root "ArhqenCognitionEngine.sln"))) { throw "Missing ArhqenCognitionEngine.sln" }
if (-not (Test-Path (Join-Path $Root "Source\ArhqenCognitionEngine.vcxproj"))) { throw "Missing Source\ArhqenCognitionEngine.vcxproj" }
if (-not (Test-Path (Join-Path $Root "Docs\ACE_CLEAN0.md"))) { throw "Missing Docs\ACE_CLEAN0.md" }

$ForbiddenPaths = @(
    "Source\Private\Cognitive",
    "Source\Public\ArhqenCognitionEngine\Cognitive",
    "Source\Private\Memory",
    "Source\Public\ArhqenCognitionEngine\Memory",
    "Source\Private\Core\CognitiveUiBridge.cpp",
    "Source\Public\ArhqenCognitionEngine\Core\CognitiveUiBridge.h",
    "Source\Private\Ui\ChatBoxUi.cpp",
    "Source\Public\ArhqenCognitionEngine\Ui\ChatBoxUi.h"
)
foreach ($rel in $ForbiddenPaths) {
    if (Test-Path (Join-Path $Root $rel)) { throw "Legacy path still exists: $rel" }
}

$ActiveFiles = Get-ChildItem (Join-Path $Root "Source"), (Join-Path $Root "Config"), (Join-Path $Root "Scripts") -Recurse -File |
    Where-Object { $_.Extension -in @(".h", ".cpp", ".rc", ".vcxproj", ".filters", ".sln", ".ps1", ".amconfig", ".md") }
$ActiveFiles += Get-Item (Join-Path $Root "CMakeLists.txt"), (Join-Path $Root "README.md")

function Assert-NoActiveText($Needle) {
    $hits = @()
    foreach ($file in $ActiveFiles) {
        $content = Get-Content $file.FullName -Raw -ErrorAction SilentlyContinue
        if ($content -like "*$Needle*") {
            $hits += $file.FullName.Substring($Root.Length + 1)
        }
    }
    if ($hits.Count -gt 0) { throw "Forbidden active text '$Needle' found in: $($hits -join ', ')" }
}

Assert-NoActiveText "CognitiveStore"
Assert-NoActiveText "CognitiveUiBridge"
Assert-NoActiveText "BeliefLedger"
Assert-NoActiveText "QuestionGenerator"
Assert-NoActiveText "ACE shell backend placeholder"
Assert-NoActiveText "CognitiveMemoryStore"
Assert-NoActiveText "ArchitectMind"
Assert-NoActiveText "Arhqen AI"

$all = ($ActiveFiles | ForEach-Object { Get-Content $_.FullName -Raw -ErrorAction SilentlyContinue }) -join "`n"
if ($all -notlike "*Arhqen Cognition Engine*") { throw "Brand string missing." }
if ($all -notlike "*3D Cognitive Environment*") { throw "3D Cognitive Environment placeholder missing." }
if ($all -notlike "*The 3D sandbox is not implemented yet*") { throw "3D placeholder body text missing." }

Write-Host "PASS|ace_clean0_solution_exists|ArhqenCognitionEngine.sln"
Write-Host "PASS|ace_clean0_project_exists|Source\ArhqenCognitionEngine.vcxproj"
Write-Host "PASS|ace_clean0_branding|Arhqen Cognition Engine"
Write-Host "PASS|ace_clean0_legacy_ai_removed|legacy backend files absent"
Write-Host "PASS|ace_clean0_environment_placeholder|3D Cognitive Environment"
