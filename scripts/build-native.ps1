param(
  [ValidateSet('Debug', 'Release')]
  [string]$Configuration = 'Debug',
  [switch]$Run,
  [switch]$Test
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $projectRoot 'build-local'
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = if ($cmakeCommand) { $cmakeCommand.Source } else { $null }

if (-not $cmakePath) {
  $vsLocator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path -LiteralPath $vsLocator) {
    $candidates = @(& $vsLocator -latest -products * -find 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
    if ($candidates.Count -gt 0) { $cmakePath = $candidates[0] }
  }
}

if (-not $cmakePath) {
  throw 'CMake was not found. Visual Studio needs Desktop development with C++, Windows SDK, and C++ CMake tools for Windows. This script does not install anything.'
}

& $cmakePath -S $projectRoot -B $buildPath -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw 'Native CMake configuration failed.' }
& $cmakePath --build $buildPath --config $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Native build failed.' }

if ($Test) {
  $ctestPath = Join-Path (Split-Path -Parent $cmakePath) 'ctest.exe'
  if (-not (Test-Path -LiteralPath $ctestPath)) { throw 'CTest was not found beside CMake.' }
  & $ctestPath --test-dir $buildPath -C $Configuration --output-on-failure
  if ($LASTEXITCODE -ne 0) { throw 'Simulation tests failed.' }
}

if ($Run) {
  $executablePath = Join-Path $buildPath "$Configuration\RPGNative.exe"
  & $executablePath
  if ($LASTEXITCODE -ne 0) { throw "Native application exited with code $LASTEXITCODE." }
}
