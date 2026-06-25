$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Header = Get-Content (Join-Path $Root "Source\Public\ArhqenCognitionEngine\Ui\AceShellUi.h") -Raw
$Cpp = Get-Content (Join-Path $Root "Source\Private\Ui\AceShellUi.cpp") -Raw

$Markers = @{
    "subtitle_spacing" = "subtitle gets real breathing room"
    "grouped_control_tabs" = "grouped control tabs"
    "scenario_group" = "renderControlGroup(scenarioGroup"
    "planner_group" = "renderControlGroup(plannerGroup"
    "runtime_group" = "renderControlGroup(runtimeGroup"
    "manual_group" = "renderControlGroup(manualGroup"
    "dashboard_after_controls" = "dashboard starts after grouped controls"
    "manual_actions_label" = "Manual Actions"
    "scenario_value_inside_group" = "widen(snapshot.scenarioName)"
    "planner_value_inside_group" = "widen(snapshot.plannerName)"
}

foreach ($name in $Markers.Keys) {
    $needle = $Markers[$name]
    if (($Header -notlike "*$needle*") -and ($Cpp -notlike "*$needle*")) {
        throw "Missing ACE-UI1R4 marker: $name -> $needle"
    }
    Write-Host "PASS|$name|$needle"
}

if ($Cpp -like "*Scenario: *" -or $Cpp -like "*Planner: * | Step=*") {
    throw "Old overlapping scenario/planner text labels still present."
}
Write-Host "PASS|old_overlapping_labels_removed"

if ($Cpp -like "*Panda3D*" -or $Cpp -like "*DearPyGui*" -or $Cpp -like "*python*" -or $Cpp -like "*Python*") {
    throw "Forbidden non-goal reference found in UI patch source."
}
Write-Host "PASS|no_forbidden_runtime_refs"

Write-Host "PASS|ace_ui1r4_static_validation|header spacing and grouped controls markers present"
