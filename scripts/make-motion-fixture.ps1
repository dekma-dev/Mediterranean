# Generates our original test asset. No external files, downloads or tools.
param([switch]$Mesh)
$ErrorActionPreference = 'Stop'
$fixtureRoot = Join-Path (Split-Path -Parent $PSScriptRoot) 'assets/characters'
$dataStream = [IO.MemoryStream]::new()
$dataWriter = [IO.BinaryWriter]::new($dataStream)
$views = [Collections.Generic.List[object]]::new()
$accessors = [Collections.Generic.List[object]]::new()
function Add-Values([string]$type, [single[]]$values, [int]$components, [switch]$Time) {
  $offset = $dataStream.Position
  foreach ($value in $values) { $dataWriter.Write($value) }
  $view = $views.Count
  $views.Add([ordered]@{buffer=0; byteOffset=$offset; byteLength=$values.Length*4})
  $entry = [ordered]@{bufferView=$view; componentType=5126; count=$values.Length/$components; type=$type}
  if ($Time) { $entry.min=@($values[0]); $entry.max=@($values[$values.Length-1]) }
  $index = $accessors.Count
  $accessors.Add($entry)
  return $index
}
function Add-Shorts([string]$type, [uint16[]]$values, [int]$components) {
  while ($dataStream.Position % 4) { $dataWriter.Write([byte]0) }
  $offset = $dataStream.Position
  foreach ($value in $values) { $dataWriter.Write($value) }
  $view = $views.Count
  $views.Add([ordered]@{buffer=0;byteOffset=$offset;byteLength=$values.Length*2})
  $index = $accessors.Count
  $accessors.Add([ordered]@{bufferView=$view;componentType=5123;count=$values.Length/$components;type=$type})
  return $index
}
try {
  $binds = Add-Values MAT4 @(1,0,0,0,0,1,0,0,0,0,1,0,0,-1,-2,1, 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1) 16
  $rotationTime = Add-Values SCALAR @(0,1) 1 -Time
  $rotation = Add-Values VEC4 @(0,0,0,1,0,0.70710678118,0,0.70710678118) 4
  $translationTime = Add-Values SCALAR @(0.25,0.75) 1 -Time
  $translation = Add-Values VEC3 @(0,1,2,2,1,4) 3
  $scaleTime = Add-Values SCALAR @(0,0.5,1) 1 -Time
  $scale = Add-Values VEC3 @(1,1,1,2,3,4,1,1,1) 3
  if ($Mesh) {
    $positions = Add-Values VEC3 @(-0.25,0,2, 0.25,0,2, -0.25,1,2, 0.25,1,2, -0.25,2,2, 0.25,2,2) 3
    $accessors[$positions].min=@(-0.25,0,2); $accessors[$positions].max=@(0.25,2,2)
    $normals = Add-Values VEC3 @(0,0,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1) 3
    $weights = Add-Values VEC4 @(0,1,0,0, 0,1,0,0, 0.5,0.5,0,0, 0.5,0.5,0,0, 1,0,0,0, 1,0,0,0) 4
    $jointIds = Add-Shorts VEC4 @(0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0) 4
    $indices = Add-Shorts SCALAR @(0,1,2,1,3,2,2,3,4,3,5,4) 1
  }
  $document = [ordered]@{
    asset=[ordered]@{version='2.0';generator='Mediterranean original motion fixture'}
    nodes=@([ordered]@{name='root';children=@(1)},[ordered]@{name='hinge';translation=@(0,1,2)})
    skins=@([ordered]@{joints=@(1,0);skeleton=0;inverseBindMatrices=$binds})
    buffers=@([ordered]@{byteLength=$dataStream.Length})
    bufferViews=$views.ToArray();accessors=$accessors.ToArray()
    animations=@([ordered]@{
      name='hinge_motion'
      samplers=@(
        [ordered]@{input=$rotationTime;output=$rotation;interpolation='LINEAR'},
        [ordered]@{input=$translationTime;output=$translation},
        [ordered]@{input=$scaleTime;output=$scale})
      channels=@(
        [ordered]@{sampler=0;target=[ordered]@{node=1;path='rotation'}},
        [ordered]@{sampler=1;target=[ordered]@{node=1;path='translation'}},
        [ordered]@{sampler=2;target=[ordered]@{node=1;path='scale'}})
    })
  }
  if ($Mesh) {
    $document.nodes += [ordered]@{name='weighted_strip';mesh=0;skin=0}
    $document.meshes=@([ordered]@{name='original_hinge_strip';primitives=@([ordered]@{
      attributes=[ordered]@{POSITION=$positions;NORMAL=$normals;JOINTS_0=$jointIds;WEIGHTS_0=$weights}
      indices=$indices;mode=4
    })})
  }
  $json = $document | ConvertTo-Json -Depth 12 -Compress
  while ($json.Length % 4) { $json += ' ' }
  $outputStream = [IO.MemoryStream]::new()
  $outputWriter = [IO.BinaryWriter]::new($outputStream)
  try {
    $outputWriter.Write([uint32]0x46546c67); $outputWriter.Write([uint32]2)
    $outputWriter.Write([uint32](28+$json.Length+$dataStream.Length))
    $outputWriter.Write([uint32]$json.Length); $outputWriter.Write([uint32]0x4e4f534a)
    $outputWriter.Write([Text.Encoding]::ASCII.GetBytes($json))
    $outputWriter.Write([uint32]$dataStream.Length); $outputWriter.Write([uint32]0x004e4942)
    $outputWriter.Write($dataStream.ToArray())
    $filename = if ($Mesh) { 'hinge-mesh.glb' } else { 'hinge-motion.glb' }
    [IO.File]::WriteAllBytes((Join-Path $fixtureRoot $filename),$outputStream.ToArray())
  } finally { $outputWriter.Dispose(); $outputStream.Dispose() }
} finally { $dataWriter.Dispose(); $dataStream.Dispose() }
