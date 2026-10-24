# Build from any directory: .\Tools\Orbit\Build-Windows.ps1 -Configuration Release -Jobs 6 -Portable
# Use -CleanFirst after a compiler update; -BuildDirectory selects another build folder.
# Use a fresh -BuildDirectory if an earlier CMake cache detected a different console encoding.
# Requires Git and Visual Studio 2026 C++ tools, Windows SDK, CMake and Ninja components.
# Source/PCH/pch.h defines the authoritative compiler/SDK minimums; this script never bypasses them.
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [ValidateRange(1, 64)][int]$Jobs = 6,
    [switch]$SkipSubmodules,
    [switch]$Portable,
    [switch]$CleanFirst,
    [string]$BuildDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'This helper requires Windows.' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer/vswhere.exe was not found.' }
$installation = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -latest -property installationPath
if ($LASTEXITCODE -ne 0 -or !$installation) { throw 'No Visual Studio installation with C++ x64 tools was found.' }
$installation = ([string]($installation | Select-Object -First 1)).Trim()
$vsdev = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$cmake = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninja = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$git = (Get-Command git.exe -CommandType Application -ErrorAction Stop).Source
foreach ($required in @($vsdev, $cmake, $ninja)) {
    if (!(Test-Path -LiteralPath $required -PathType Leaf)) { throw "Required Visual Studio component is missing: $required" }
}
$configurationName = $Configuration.ToLowerInvariant()
$buildDirectoryPath = if ($BuildDirectory) {
    [IO.Path]::GetFullPath($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDirectory))
} else {
    Join-Path $repository "build\$configurationName\x64"
}
$batchPath = Join-Path ([IO.Path]::GetTempPath()) ('dolphin-orbit-' + [guid]::NewGuid().ToString('N') + '.cmd')
$paths = @{
    ORBIT_BUILD_BATCH = $batchPath; ORBIT_BUILD_ROOT = $repository
    ORBIT_BUILD_VSDEV = $vsdev; ORBIT_BUILD_CMAKE = $cmake
    ORBIT_BUILD_NINJA = [IO.Path]::GetDirectoryName($ninja); ORBIT_BUILD_GIT = $git
    ORBIT_BUILD_DIRECTORY = $buildDirectoryPath
}
foreach ($path in $paths.Values) {
    if ($path -match '[\r\n"]') { throw 'Build paths cannot contain line breaks or double quotes.' }
}
# Keep paths in the child environment: CMD expands them once, inside quotes, without delayed expansion.
# CALL expands its argument a second time, so protect literal percent signs in the VsDevCmd path.
$paths.ORBIT_BUILD_VSDEV = $vsdev.Replace('%', '%%')
$submodules = if ($SkipSubmodules) { '' } else {
    '"%ORBIT_BUILD_GIT%" -c core.longpaths=true submodule update --init --recursive --depth 1 --jobs 6'
}
$cleanFirstArgument = if ($CleanFirst) { '--clean-first' } else { '' }
$batch = @"
@echo off
setlocal DisableDelayedExpansion
call "%ORBIT_BUILD_VSDEV%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
chcp 65001 >nul
if errorlevel 1 exit /b %errorlevel%
set "PATH=%ORBIT_BUILD_NINJA%;%PATH%"
cd /d "%ORBIT_BUILD_ROOT%"
if errorlevel 1 exit /b %errorlevel%
$submodules
if errorlevel 1 exit /b %errorlevel%
"%ORBIT_BUILD_CMAKE%" --preset ninja-$configurationName-x64 -B "%ORBIT_BUILD_DIRECTORY%" -DDISTRIBUTOR=Orbit -DENABLE_AUTOUPDATE=OFF -DENABLE_TESTS=OFF
if errorlevel 1 exit /b %errorlevel%
"%ORBIT_BUILD_CMAKE%" --build "%ORBIT_BUILD_DIRECTORY%" --target dolphin-emu --parallel $Jobs $cleanFirstArgument
exit /b %errorlevel%
"@
Write-Host 'Compiler and SDK requirements are enforced by Source/PCH/pch.h (currently MSVC 19.51.36252+, SDK 10.0.22621+).'
$process = $null
try {
    [IO.File]::WriteAllText($batchPath, $batch, [Text.Encoding]::ASCII)
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $env:ComSpec
    $start.Arguments = '/d /s /c ""%ORBIT_BUILD_BATCH%""'
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    # Match the child console encoding so localized /showIncludes diagnostics and
    # CMake's cached Ninja dependency prefix use the same bytes throughout the build.
    $start.StandardOutputEncoding = [Text.Encoding]::UTF8
    $start.StandardErrorEncoding = [Text.Encoding]::UTF8
    foreach ($entry in $paths.GetEnumerator()) { $start.EnvironmentVariables[$entry.Key] = $entry.Value }
    $process = [Diagnostics.Process]::Start($start)
    # Drain both pipes concurrently, forwarding diagnostics to PowerShell in real time.
    # Inheriting console handles loses child output when this script is redirected to a log.
    $stdoutTask = $process.StandardOutput.ReadLineAsync()
    $stderrTask = $process.StandardError.ReadLineAsync()
    while ($null -ne $stdoutTask -or $null -ne $stderrTask) {
        $pending = @($stdoutTask, $stderrTask) | Where-Object { $null -ne $_ }
        [void][Threading.Tasks.Task]::WaitAny([Threading.Tasks.Task[]]$pending)
        if ($null -ne $stdoutTask -and $stdoutTask.IsCompleted) {
            $line = $stdoutTask.GetAwaiter().GetResult()
            if ($null -eq $line) { $stdoutTask = $null } else {
                Write-Output $line
                $stdoutTask = $process.StandardOutput.ReadLineAsync()
            }
        }
        if ($null -ne $stderrTask -and $stderrTask.IsCompleted) {
            $line = $stderrTask.GetAwaiter().GetResult()
            if ($null -eq $line) { $stderrTask = $null } else {
                Write-Output $line
                $stderrTask = $process.StandardError.ReadLineAsync()
            }
        }
    }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "Build failed with exit code $($process.ExitCode)." }
} finally {
    if ($null -ne $process) { $process.Dispose() }
    if (Test-Path -LiteralPath $batchPath) { Remove-Item -LiteralPath $batchPath -Force }
}
$binaries = Join-Path $buildDirectoryPath 'Binaries'
$executable = Join-Path $binaries $(if ($Configuration -eq 'Debug') { 'DolphinD.exe' } else { 'Dolphin.exe' })
if (!(Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Expected executable was not found: $executable" }
if ($Portable) { [IO.File]::WriteAllText((Join-Path $binaries 'portable.txt'), '') }
Write-Host "Build ready: $executable"
