param(
    [string]$Solution = "ArhqenCognitionEngine.sln"
)

$ErrorActionPreference = "Stop"

Write-Host "=== Arhqen Cognition Engine ACE-CLEAN0 Debug Build - MSBuild ==="

function Find-MSBuild {
    $direct = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($direct) { return $direct.Source }

    $vswhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )

    foreach ($candidate in $vswhereCandidates) {
        if (Test-Path $candidate) {
            $installPath = & $candidate -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
            if ($installPath) {
                $msbuild = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
                if (Test-Path $msbuild) { return $msbuild }
            }
        }
    }

    throw "MSBuild was not found. Open Developer PowerShell or install Visual Studio C++ workload."
}

$msbuild = Find-MSBuild
Write-Host "MSBuild: $msbuild"

& $msbuild $Solution /m /p:Configuration=Debug /p:Platform=x64
if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed with exit code $LASTEXITCODE."
}

Write-Host "Build complete."
