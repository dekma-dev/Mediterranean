# Run with PowerShell 7. Uses only installed Windows D3D runtime components.
param([switch]$Preview)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$header=[IO.File]::ReadAllText((Join-Path $projectRoot 'engine\SceneShader.h'))
# C++ concatenates the raw literals used to stay below MSVC's token limit.
# Reconstruct that same source for standalone D3D diagnostics.
$parts=[regex]::Matches($header,'(?s)R"\((.*?)\)"')
if($parts.Count -eq 0) { throw 'SceneShader.h raw strings not found.' }
foreach($part in $parts) {
 if([Text.Encoding]::UTF8.GetByteCount($part.Groups[1].Value) -gt 16000) {
  throw 'Split the SceneShader.h raw literal before it exceeds MSVC''s 16 KiB token limit.'
 }
}
$source=($parts | ForEach-Object { $_.Groups[1].Value }) -join ''
Add-Type -Path (Join-Path $PSScriptRoot 'SceneShaderCheck.cs')
$bytecode=@{}
foreach($entry in @('VSMain','PSMain','VSWorld','VSShadow','VSBody','VSBodyShadow','VSSky','PSSky','VSEffect','PSEffect')) {
 $profile=if($entry.StartsWith('VS')) {'vs_5_0'} else {'ps_5_0'}
 $bytecode[$entry]=[SceneShaderCheck]::Compile($source,$entry,$profile)
 Write-Output ("PASS {0}: {1} bytes, no compiler diagnostics" -f $entry,$bytecode[$entry].Length)
}
if($Preview) {
 $outputPath=Join-Path $projectRoot 'build-shader-check'
 [IO.Directory]::CreateDirectory($outputPath) | Out-Null
 $sunYaw=[Math]::Atan2(-30,-24)
 $sunElevation=[Math]::Atan2(51,[Math]::Sqrt(30*30+24*24))
 $views=@(
  @{Name='sun';Yaw=$sunYaw;Elevation=$sunElevation;Seconds=0},
  @{Name='horizon';Yaw=$sunYaw;Elevation=0.12;Seconds=0},
  @{Name='away';Yaw=$sunYaw+[Math]::PI;Elevation=0.4;Seconds=0},
  @{Name='clouds';Yaw=0;Elevation=0.5;Seconds=0},
  @{Name='clouds-later';Yaw=0;Elevation=0.5;Seconds=180}
 )
 $images=@{}
 foreach($view in $views) {
  $path=Join-Path $outputPath ('sky-'+$view.Name+'.png')
  $pixels=[SceneShaderCheck]::Sky($bytecode['VSSky'],$bytecode['PSSky'],960,540,
    $view.Yaw,$view.Elevation,$view.Seconds,$path)
  $images[$view.Name]=$pixels
  Write-Output ("Rendered {0} ({1} bytes RGBA) with D3D11 WARP" -f $path,$pixels.Length)
 }
 $center=(270*960+480)*4
 if($images['sun'][$center] -lt 240 -or $images['sun'][$center+1] -lt 240) { throw 'Sun disc is not aligned with the shared light direction.' }
 $change=[SceneShaderCheck]::Difference($images['clouds'],$images['clouds-later'])
 if($change -le 10) { throw 'Cloud animation did not change sky pixels.' }
 $period=[SceneShaderCheck]::Sky($bytecode['VSSky'],$bytecode['PSSky'],960,540,0,0.5,1800,(Join-Path $outputPath 'sky-cloud-period.png'))
 if([SceneShaderCheck]::Difference($images['clouds'],$period) -ne 0) { throw 'Cloud period contains a discontinuity.' }
 Write-Output ("PASS sky probes: aligned sun, cloud movement (max channel change {0}), exact closed drift period" -f $change)

 # Clear and optically thick skies must transmit/block direct sunlight without NaN.
 # The probe uses production density/light functions, not a CPU reimplementation.
 $probeSource=$source+@'
float4 PSAtmosphereProbe(SkyOut i) : SV_TARGET {
 float3 ray=SunDirection();
 float4 clouds=CloudRadiance(ray,.004,.5);
 float direct=CloudSunlight(float3(0,0,0));
 bool finite=all(isfinite(clouds)) && isfinite(direct) && all(isfinite(SkyRadiance(ray)));
 return float4(clouds.a,direct,finite,1);
}
'@
 $probe=[SceneShaderCheck]::Compile($probeSource,'PSAtmosphereProbe','ps_5_0')
 $worst=0
 foreach($phase in 0..23) {
  $pixels=[SceneShaderCheck]::Sky($bytecode['VSSky'],$probe,1,1,0,0,($phase*75),(Join-Path $outputPath 'sky-light-probe.png'))
  if($pixels[2] -ne 255) { throw 'Non-finite atmosphere response.' }
  $worst=[Math]::Max($worst,[Math]::Abs([int]$pixels[0]-[int]$pixels[1]))
 }
 if($worst -gt 40) { throw "Cloud shadow disagrees with sky opacity: $worst / 255." }
 foreach($threshold in @(2,-1)) {
  $pixels=[SceneShaderCheck]::Sky($bytecode['VSSky'],$probe,1,1,0,0,0,(Join-Path $outputPath 'sky-coverage-probe.png'),$threshold)
  $expected=if($threshold -eq 2) {255} else {0}
  if($pixels[2] -ne 255 -or $pixels[0] -ne $expected -or $pixels[1] -ne $expected) {
   throw 'Clear/opaque clouds do not transmit/block the sun consistently.'
  }
 }
 $next=[SceneShaderCheck]::Sky($bytecode['VSSky'],$bytecode['PSSky'],960,540,0,0.5,(1.0/60),(Join-Path $outputPath 'sky-cloud-next-frame.png'))
 $mean=[SceneShaderCheck]::MeanDifference($images['clouds'],$next)
 if($mean -gt .15) { throw "Cloud drift flickers between 60 Hz updates: mean RGB change $mean." }
 $clear=[SceneShaderCheck]::Sky($bytecode['VSSky'],$bytecode['PSSky'],960,540,$sunYaw,$sunElevation,0,(Join-Path $outputPath 'sky-clear-sun.png'),2)
 $overcast=[SceneShaderCheck]::Sky($bytecode['VSSky'],$bytecode['PSSky'],960,540,$sunYaw,$sunElevation,0,(Join-Path $outputPath 'sky-covered-sun.png'),-1)
 # A dense deck must remove the compact solar core, not leave a bright pinhole.
 $neighbor=(270*960+510)*4
 if([Math]::Abs([int]$overcast[$center]-[int]$overcast[$neighbor]) -gt 4) { throw 'The solar core leaks through opaque cloud.' }
 if([int]$clear[$center]-[int]$clear[$neighbor] -lt 5) { throw 'The clear-sky sun has lost its compact core.' }
 Write-Output ("PASS cloud volume probes: clear/opaque sun, 24-phase light agreement (max {0}/255), 60 Hz mean RGB change {1:F4}" -f $worst,$mean)

 # Large ground faces interpolate vertex lighting; keep the cloud footprint broad
 # enough that both island diagonals approximate the direct center sample.
 $interpolationSource=$source+@'
float4 PSInterpolationProbe(SkyOut i) : SV_TARGET {
 float center=CloudSunlight(float3(0,0,0));
 float a=CloudSunlight(float3(-21,0,-23)),b=CloudSunlight(float3(21,0,23));
 float x=CloudSunlight(float3(-21,0,23)),y=CloudSunlight(float3(21,0,-23));
 return float4(abs((a+b)*.5-center),abs((x+y)*.5-center),1,1);
}
'@
 $interpolation=[SceneShaderCheck]::Compile($interpolationSource,'PSInterpolationProbe','ps_5_0')
 $interpolationError=0
 foreach($phase in 0..23) {
  $pixels=[SceneShaderCheck]::Sky($bytecode['VSSky'],$interpolation,1,1,0,0,($phase*75),(Join-Path $outputPath 'sky-interpolation-probe.png'))
  $interpolationError=[Math]::Max($interpolationError,[Math]::Max([int]$pixels[0],[int]$pixels[1]))
 }
 if($interpolationError -gt 20) { throw "Cloud lighting varies too quickly for large ground faces: $interpolationError / 255." }
 Write-Output ("PASS broad ground lighting: maximum diagonal interpolation error {0}/255 across 24 phases" -f $interpolationError)
}
