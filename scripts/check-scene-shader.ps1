# Run with PowerShell 7. Uses only installed Windows D3D runtime components.
param([switch]$Preview)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$header=[IO.File]::ReadAllText((Join-Path $projectRoot 'engine\SceneShader.h'))
$start=$header.IndexOf('R"(')+3
$end=$header.LastIndexOf(')";')
if($start -lt 3 -or $end -lt $start) { throw 'SceneShader.h raw string not found.' }
$source=$header.Substring($start,$end-$start)
Add-Type -Path (Join-Path $PSScriptRoot 'SceneShaderCheck.cs')
$bytecode=@{}
foreach($entry in @('VSMain','PSMain','VSWorld','VSShadow','VSSky','PSSky','VSEffect','PSEffect')) {
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
}
