#include "engine/SceneShader.h"
#include "tools/ShaderContract.h"
#include <set>
#include <iostream>
#include <cstring>

namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::set<std::string> resourceCoverage;
std::vector<std::string> Compile(const std::string& source,const char* entry) {
  Microsoft::WRL::ComPtr<ID3DBlob> code,errors;
  const auto hr=D3DCompile(source.data(),source.size(),"ContractRegression",nullptr,nullptr,entry,
    entry[0]=='V'?"vs_5_0":"ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors);
  if(FAILED(hr))throw std::runtime_error(errors?static_cast<const char*>(errors->GetBufferPointer()):"D3DCompile failed");
  auto buffers=shader_contract::Validate(code->GetBufferPointer(),code->GetBufferSize(),entry);
  Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
  Check(SUCCEEDED(D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),IID_PPV_ARGS(&reflection))),"Coverage reflection");
  D3D11_SHADER_DESC description{};reflection->GetDesc(&description);
  for(unsigned i=0;i<description.BoundResources;++i){D3D11_SHADER_INPUT_BIND_DESC binding{};reflection->GetResourceBindingDesc(i,&binding);if(binding.Type!=D3D_SIT_CBUFFER)resourceCoverage.insert(binding.Name);}
  return buffers;
}
void Reject(const char* entry,const char* before,const char* after,const char* detail) {
  std::string source=sceneShader;const auto at=source.find(before);Check(at!=std::string::npos,"Missing shader mutation target");
  source.replace(at,std::strlen(before),after);
  bool rejected=false;
  try{Compile(source,entry);}catch(const std::runtime_error& e){rejected=std::string(e.what()).find(detail)!=std::string::npos;}
  Check(rejected,"Shader ABI regression was not rejected with the expected diagnostic");
}
void RejectSnippet(const char* source,const char* diagnostic){
  bool rejected=false;try{Compile(source,"PSMain");}catch(const std::runtime_error& e){rejected=std::string(e.what()).find(diagnostic)!=std::string::npos;}
  Check(rejected,"Synthetic resource contract violation was not caught");
}
}
int main()try {
  std::set<std::string> buffers;
  for(const char* entry:{"VSMain","PSMain","VSWorld","VSShadow","VSBody","VSBodyShadow","VSHand","VSHandShadow","VSSky","PSSky","PSCloudLight","VSEffect","PSEffect","VSGlow","PSGlowSource","PSGlowHorizontal","PSGlowVertical","PSGlowComposite","PSPressure"})
    for(auto& name:Compile(sceneShader,entry))buffers.insert(name);
  Check(buffers.size()==shader_contract::Layouts().size(),"Some GPU upload layouts were never reflected");
  Check(resourceCoverage.size()==shader_contract::Resources().size(),"Some texture/sampler/structured resources were never reflected");
  Reject("VSMain","float4 color; float4 eyeAndHud;","float4 eyeAndHud; float4 color;","Scene.color");
  Reject("VSMain","matrix wvp;","row_major matrix wvp;","Scene.wvp");
  Reject("VSMain","shadowVP[2]","shadowVP[3]","CPU buffer size");
  Reject("VSMain","Scene : register(b0)","Scene : register(b1)","register mismatch");
  Reject("PSEffect","float4 rightRadius; float4 upRadius;","float4 upRadius; float4 rightRadius;","rightRadius");
  Reject("PSEffect","spellVolumes[24]","spellVolumes[25]","CPU buffer size");
  Reject("PSPressure","float4 radiiAge;float4 axisStyle;","float4 axisStyle;float4 radiiAge;","radiiAge");
  Reject("PSMain","fontAtlas : register(t2)","fontAtlas : register(t22)","fontAtlas: resource register");
  Reject("PSMain","fontSampler : register(s2)","fontSampler : register(s3)","fontSampler: resource register");
  Reject("VSBody","StructuredBuffer<float4> bodyRows","Buffer<float4> bodyRows","bodyRows: resource kind");
  Reject("VSMain","float3 n : NORMAL;","float3 n : TANGENT;","missing CPU vertex semantic");
  Reject("VSMain","float4 tint : COLOR;","float4 tint : COLOR1;","missing CPU vertex semantic");
  Reject("VSBody","uint4 bones : BLENDINDICES;","int4 bones : BLENDINDICES;","vertex component type");
  RejectSnippet("StructuredBuffer<float2> bodyRows : register(t9); float4 PSMain() : SV_TARGET { return float4(bodyRows[0],0,1); }","structured element stride");
  RejectSnippet("Texture2D<float4> alien : register(t31); float4 PSMain() : SV_TARGET { return alien.Load(int3(0,0,0)); }","unregistered resource");
  RejectSnippet("Texture2DArray<float> shadowMap : register(t1); SamplerComparisonState fontSampler : register(s2); float4 PSMain() : SV_TARGET {return float4(shadowMap.SampleCmpLevelZero(fontSampler,float3(.5,.5,0),.5),0,0,1);}","sampler comparison mode");
  RejectSnippet("Texture2D<uint4> mapArtwork : register(t3); float4 PSMain() : SV_TARGET {return float4(mapArtwork.Load(int3(0,0,0)));}","texture return type");
  std::cout<<"PASS: 19 shader stages, 8 constant buffers, 21 SRVs/3 samplers, vertex signatures, structured stride and negative ABI mutations\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
