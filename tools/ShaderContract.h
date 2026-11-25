#pragma once
#include "engine/ShaderData.h"
#include "engine/HandSkinning.h"
#include "engine/TransientLights.h"
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <stdexcept>

namespace shader_contract {
struct Member {
  const char* name; unsigned offset,bytes,rows=1,elements=0;
  std::vector<Member> members;
};
struct Buffer {const char* name;unsigned slot,bytes;std::vector<Member> members;};
#define FIELD(type, field) Member{#field,unsigned(offsetof(type,field)),unsigned(sizeof(type::field))}
#define NAMED(type, field, name) Member{name,unsigned(offsetof(type,field)),unsigned(sizeof(type::field))}
#define MATRIX(type, field) Member{#field,unsigned(offsetof(type,field)),unsigned(sizeof(type::field)),4}
inline std::vector<Buffer> Layouts() {
  using namespace shader;
  return {
    {"Scene",slot::scene,sizeof(Scene),{MATRIX(Scene,world),MATRIX(Scene,wvp),MATRIX(Scene,normalWorld),
      {"shadowVP",offsetof(Scene,shadowVP),sizeof(Scene::shadowVP),4,visual::shadowLayers},
      FIELD(Scene,color),FIELD(Scene,eyeAndHud),FIELD(Scene,sun),FIELD(Scene,atmosphere)}},
    {"Hands",slot::hands,sizeof(visual::HandPalette)*visual::maxHands,
      {{"handJoints",0,sizeof(visual::HandPalette)*visual::maxHands,4,visual::handBones*visual::maxHands}}},
    {"CloudField",slot::cloud,sizeof(XMFLOAT4),{{"cloudField",0,sizeof(XMFLOAT4)}}},
    {"CombatLights",slot::lights,sizeof(visual::TransientLightBlock),{
      NAMED(visual::TransientLightBlock,count,"combatLightCount"),
      {"combatLights",offsetof(visual::TransientLightBlock,lights),sizeof(visual::TransientLightBlock::lights),0,4,
        {FIELD(visual::TransientLight,positionRadius),FIELD(visual::TransientLight,colorPower)}}}},
    {"SpellVolumes",slot::volumes,sizeof(Volumes),{{"spellVolumes",0,sizeof(Volumes),0,unsigned(std::tuple_size_v<Volumes>),{
      FIELD(Volume,centerKind),FIELD(Volume,rightRadius),FIELD(Volume,upRadius),FIELD(Volume,axisRadius),FIELD(Volume,clock),FIELD(Volume,tint)}}}},
    {"SpellGlow",slot::glow,sizeof(Glow),{NAMED(Glow,texel,"glowTexel"),NAMED(Glow,screen,"glowScreen"),NAMED(Glow,region,"glowRegion")}},
    {"SpellFields",slot::fields,sizeof(Fields),{{"spellFields",0,sizeof(Fields),0,unsigned(std::tuple_size_v<Fields>),{
      FIELD(Field,clock),FIELD(Field,tint),FIELD(Field,shape),FIELD(Field,details)}}}},
    {"PressureScene",slot::pressure,sizeof(Pressure),{
      {"pressureInverseVP",offsetof(Pressure,inverseVP),sizeof(Pressure::inverseVP),4},
      {"pressureVP",offsetof(Pressure,vp),sizeof(Pressure::vp),4},
      NAMED(Pressure,eye,"pressureEye"),NAMED(Pressure,screen,"pressureScreen"),NAMED(Pressure,region,"pressureRegion"),
      {"pressureWaves",offsetof(Pressure,waves),sizeof(Pressure::waves),0,unsigned(std::tuple_size_v<PressureWaves>),{
        FIELD(PressureWave,centerStrength),FIELD(PressureWave,radiiAge),FIELD(PressureWave,axisStyle)}}}}
  };
}
#undef MATRIX
#undef NAMED
#undef FIELD
inline void Require(bool condition,const std::string& path,const char* detail) {
  if(!condition)throw std::runtime_error("Shader contract: "+path+": "+detail);
}
inline void Type(ID3D11ShaderReflectionType* actual,const Member& expected,const std::string& path,bool nested=false) {
  D3D11_SHADER_TYPE_DESC d{};
  Require(actual && SUCCEEDED(actual->GetDesc(&d)),path,"missing type");
  Require(d.Elements==expected.elements,path,"array length mismatch");
  if(nested)Require(d.Offset==expected.offset,path,"member offset mismatch");
  if(!expected.members.empty()) {
    Require(d.Class==D3D_SVC_STRUCT && d.Members==expected.members.size(),path,"struct shape mismatch");
    for(const auto& child:expected.members)Type(actual->GetMemberTypeByName(child.name),child,path+"."+child.name,true);
  } else {
    Require(d.Class==(expected.rows==4?D3D_SVC_MATRIX_COLUMNS:D3D_SVC_VECTOR),path,"matrix packing/type mismatch");
    Require(d.Type==D3D_SVT_FLOAT && d.Rows==expected.rows && d.Columns==4,path,"scalar/vector dimensions mismatch");
  }
}
// Optimized stages can omit unused buffers. Every surviving variable (even
// unused members within a live buffer) must match the actual CPU upload type.
inline std::vector<std::string> Validate(const void* bytes,std::size_t size,const std::string& entry) {
  Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
  Require(SUCCEEDED(D3DReflect(bytes,size,IID_PPV_ARGS(&reflection))),entry,"reflection failed");
  D3D11_SHADER_DESC desc{};Require(SUCCEEDED(reflection->GetDesc(&desc)),entry,"shader description failed");
  const auto layouts=Layouts();std::vector<std::string> seen;
  for(unsigned i=0;i<desc.ConstantBuffers;++i) {
    auto* buffer=reflection->GetConstantBufferByIndex(i);D3D11_SHADER_BUFFER_DESC d{};
    Require(SUCCEEDED(buffer->GetDesc(&d)),entry,"buffer description failed");
    if(d.Type!=D3D_CT_CBUFFER)continue;
    const std::string path=entry+"/"+d.Name;
    const auto expected=std::find_if(layouts.begin(),layouts.end(),[&](const auto& b){return std::string(b.name)==d.Name;});
    Require(expected!=layouts.end(),path,"unregistered constant buffer");
    Require(d.Size==expected->bytes && d.Variables==expected->members.size(),path,"CPU buffer size/field count mismatch");
    D3D11_SHADER_INPUT_BIND_DESC binding{};
    Require(SUCCEEDED(reflection->GetResourceBindingDescByName(d.Name,&binding)),path,"missing binding");
    Require(binding.Type==D3D_SIT_CBUFFER && binding.BindPoint==expected->slot && binding.BindCount==1,path,"constant buffer register mismatch");
    for(const auto& member:expected->members) {
      auto* variable=buffer->GetVariableByName(member.name);D3D11_SHADER_VARIABLE_DESC v{};
      const auto fieldPath=path+"."+member.name;
      Require(variable && SUCCEEDED(variable->GetDesc(&v)),fieldPath,"missing variable");
      Require(v.StartOffset==member.offset && v.Size==member.bytes,fieldPath,"CPU offset/size mismatch");
      Type(variable->GetType(),member,fieldPath);
    }
    seen.emplace_back(d.Name);
  }
  return seen;
}
}
