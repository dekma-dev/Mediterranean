param(
  [ValidateSet('Debug', 'Release')]
  [string]$Configuration = 'Debug',
  [ValidateSet('VisualStudio', 'Ninja')]
  [string]$Backend = 'VisualStudio',
  [switch]$Run,
  [switch]$Test,
  [switch]$SmokeTest,
  [switch]$Benchmark,
  [Parameter(DontShow)]
  [switch]$DeveloperEnvironment
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = if ($Backend -eq 'Ninja') {
  Join-Path $projectRoot ('build-ninja-' + $Configuration.ToLowerInvariant())
} else { Join-Path $projectRoot 'build-local' }
$vsLocator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if ($Backend -eq 'Ninja' -and -not $DeveloperEnvironment) {
  if (-not (Test-Path -LiteralPath $vsLocator)) { throw 'Visual Studio locator was not found.' }
  $vsRoot = & $vsLocator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if (-not $vsRoot) { throw 'No Visual Studio installation with C++ tools was found.' }
  $developerCommand = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
  $shellPath = (Get-Process -Id $PID).Path
  # These paths are quoted for cmd.exe. Refuse expansion/metacharacters rather than
  # interpreting unusual checkout paths as shell code. No machine settings change.
  foreach ($path in @($developerCommand, $shellPath, $PSCommandPath)) {
    if ($path -match '[%"!&|<>^\r\n]') { throw 'Use Visual Studio for paths containing command-shell metacharacters.' }
  }
  $switches = @('-DeveloperEnvironment')
  if ($Run) { $switches += '-Run' }
  if ($Test) { $switches += '-Test' }
  if ($SmokeTest) { $switches += '-SmokeTest' }
  if ($Benchmark) { $switches += '-Benchmark' }
  $childCommand = 'call "' + $developerCommand + '" -no_logo -arch=x64 -host_arch=x64 && "' +
    $shellPath + '" -NoProfile -File "' + $PSCommandPath + '" -Backend Ninja -Configuration ' + $Configuration + ' ' + ($switches -join ' ')
  & $env:ComSpec /d /s /c $childCommand
  if ($LASTEXITCODE -ne 0) { throw 'Ninja build/test process failed. See the first error above.' }
  return
}
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = if ($cmakeCommand) { $cmakeCommand.Source } else { $null }

if (-not $cmakePath) {
  if (Test-Path -LiteralPath $vsLocator) {
    $candidates = @(& $vsLocator -latest -products * -find 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
    if ($candidates.Count -gt 0) { $cmakePath = $candidates[0] }
  }
}

if (-not $cmakePath) {
  throw 'CMake was not found. Visual Studio needs Desktop development with C++, Windows SDK, and C++ CMake tools for Windows. This script does not install anything.'
}

if ($Backend -eq 'Ninja') {
  if (-not (Get-Command cl -ErrorAction SilentlyContinue) -or $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw 'Ninja requires the x64 Visual Studio developer environment.'
  }
  & $cmakePath -S $projectRoot -B $buildPath -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration" -DBUILD_TESTING=ON
} else {
  & $cmakePath -S $projectRoot -B $buildPath -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
}
if ($LASTEXITCODE -ne 0) { throw 'Native CMake configuration failed.' }
& $cmakePath --build $buildPath --config $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Native build failed.' }
$executablePath = if ($Backend -eq 'Ninja') { Join-Path $buildPath 'RPGNative.exe' }
  else { Join-Path $buildPath "$Configuration\RPGNative.exe" }
Write-Output "Game executable: $executablePath"

if ($Test) {
  $ctestPath = Join-Path (Split-Path -Parent $cmakePath) 'ctest.exe'
  if (-not (Test-Path -LiteralPath $ctestPath)) { throw 'CTest was not found beside CMake.' }
  & $ctestPath --test-dir $buildPath -C $Configuration --output-on-failure
  if ($LASTEXITCODE -ne 0) { throw 'Regression tests failed.' }
}

if ($SmokeTest) {
  foreach ($scene in @('spawn', 'combat', 'captain', 'veteran')) {
    $captureName = if ($scene -eq 'spawn') { "$Configuration-smoke.png" } else { "$Configuration-$scene.png" }
    $capturePath = Join-Path $buildPath $captureName
    $arguments = @('--smoke-test', ('"' + $capturePath + '"'))
    if ($scene -ne 'spawn') {
      $snapshot = Join-Path $projectRoot "tests\fixtures\$scene-preview.save"
      $arguments += '"' + $snapshot + '"'
    }
    $renderCheck = Start-Process -FilePath $executablePath -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if (-not $renderCheck.WaitForExit(45000)) {
      $renderCheck.Kill()
      throw "Render smoke test timed out: $scene."
    }
    if (Test-Path -LiteralPath "$capturePath.log") { Get-Content -LiteralPath "$capturePath.log" }
    if ($renderCheck.ExitCode -ne 0) { throw "Render smoke test failed for $scene with code $($renderCheck.ExitCode)." }
    Write-Output "Render capture: $capturePath (world, inspection angles, panels and combined equipment/bag)"
    if ($scene -eq 'veteran') { Write-Output 'Veteran extras: outfits, sparring, quartermaster, purchased equipment and buyback. Total: 107 captures.' }
  }
}

if ($Benchmark) {
  $reportPath = Join-Path $buildPath "$Configuration-benchmark.txt"
  $measurement = Start-Process -FilePath $executablePath -ArgumentList @('--benchmark', ('"' + $reportPath + '"')) -WindowStyle Hidden -PassThru
  if (-not $measurement.WaitForExit(30000)) {
    $measurement.Kill()
    throw 'Rendering benchmark timed out.'
  }
  if ($measurement.ExitCode -ne 0) {
    if (Test-Path -LiteralPath "$reportPath.log") { Get-Content -LiteralPath "$reportPath.log" }
    throw "Rendering benchmark failed with code $($measurement.ExitCode)."
  }
  Get-Content -LiteralPath $reportPath
  Write-Output "Benchmark report: $reportPath"
}

if ($Run) {
  & $executablePath
  if ($LASTEXITCODE -ne 0) { throw "Native application exited with code $LASTEXITCODE." }
}
