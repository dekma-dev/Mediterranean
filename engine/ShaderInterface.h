#pragma once
#include "engine/Skinning.h"
#include "engine/StaticScene.h"
#include <d3d11.h>
#include <vector>
#include <cstddef>
#include <type_traits>

namespace shader {
namespace texture {
inline constexpr unsigned materials=0,shadows=1,font=2,map=3,terrain=4,frame=5,hall=6,icons=7,cloud=8,body=9,
  surfaceColor=10,surfaceNormal=11,classSpells=12,combatSpells=13,effects=14,depth=15,glow=16,priestSpells=17,
  pressure=18,surfaceColorSmall=19,surfaceNormalSmall=20,count=21;
}
namespace sampler {inline constexpr unsigned surface=0,shadow=1,font=2;}
enum class VertexStream { Basic,Instance,Skinned,Generated };
inline std::vector<D3D11_INPUT_ELEMENT_DESC> InputElements(VertexStream stream) {
  if(stream==VertexStream::Generated)return {};
  using V=visual::Vertex;using I=visual::SceneInstance;using S=visual::SkinInfluence;
  std::vector<D3D11_INPUT_ELEMENT_DESC> result={
    {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,UINT(offsetof(V,position)),D3D11_INPUT_PER_VERTEX_DATA,0},
    {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,UINT(offsetof(V,normal)),D3D11_INPUT_PER_VERTEX_DATA,0},
    {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,UINT(offsetof(V,color)),D3D11_INPUT_PER_VERTEX_DATA,0}};
  if(stream==VertexStream::Basic)return result;
  for(UINT i=0;i<4;++i)result.push_back({"WORLD",i,DXGI_FORMAT_R32G32B32A32_FLOAT,1,UINT(offsetof(I,world))+i*16,D3D11_INPUT_PER_INSTANCE_DATA,1});
  for(UINT i=0;i<4;++i)result.push_back({"INORMAL",i,DXGI_FORMAT_R32G32B32A32_FLOAT,1,UINT(offsetof(I,normal))+i*16,D3D11_INPUT_PER_INSTANCE_DATA,1});
  result.push_back({"PAINT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,UINT(offsetof(I,color)),D3D11_INPUT_PER_INSTANCE_DATA,1});
  result.push_back({"SURFACE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,UINT(offsetof(I,surface)),D3D11_INPUT_PER_INSTANCE_DATA,1});
  if(stream==VertexStream::Skinned){
    result.push_back({"BLENDINDICES",0,DXGI_FORMAT_R32G32B32A32_UINT,2,UINT(offsetof(S,bones)),D3D11_INPUT_PER_VERTEX_DATA,0});
    result.push_back({"BLENDWEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,2,UINT(offsetof(S,weights)),D3D11_INPUT_PER_VERTEX_DATA,0});
  }
  return result;
}
static_assert(std::is_standard_layout_v<visual::Vertex> && std::is_standard_layout_v<visual::SceneInstance> && std::is_standard_layout_v<visual::SkinInfluence>);
}
