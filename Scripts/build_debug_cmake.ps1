param(
    [string]$BuildDir = "Build\ACE-CLEAN0_CMake"
)

$ErrorActionPreference = "Stop"

Write-Host "=== ArhqenCognitionEngine ACE-CLEAN0 Debug Build - Optional CMake Fallback ==="

function Find-CMake {
    $direct = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($direct) { return $direct.Source }

    $vswhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )

    foreach ($candidate in $vswhereCandidates) {
        if (Test-Path $candidate) {
            $installPath = & $candidate -latest -products * -property installationPath
            if ($installPath) {
                $cmake = Join-Path $installPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
                if (Test-Path $cmake) { return $cmake }
            }
        }
    }

    throw "CMake was not found. Install CMake or use Visual Studio's bundled CMake."
}

$cmake = Find-CMake
Write-Host "CMake: $cmake"

& $cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

& $cmake --build $BuildDir --config Debug
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed with exit code $LASTEXITCODE."
}

Write-Host "Optional CMake build complete."
