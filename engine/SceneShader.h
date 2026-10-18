#pragma once
inline constexpr char sceneShader[] = R"(
cbuffer Scene : register(b0) {
 matrix world; matrix wvp; matrix normalWorld; matrix shadowVP;
 float4 color; float4 eyeAndHud;
};
Texture2DArray materials : register(t0);
Texture2D<float> shadowMap : register(t1);
Texture2D<float> fontAtlas : register(t2);
Texture2D<float4> mapArtwork : register(t3);
Texture2D<float4> terrainSurface : register(t4);
SamplerState fontSampler : register(s2);
SamplerState surfaceSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);
struct VIn { float3 p : POSITION; float3 n : NORMAL; float4 tint : COLOR; };
struct InstanceIn {
 float3 p : POSITION; float3 n : NORMAL; float4 tint : COLOR;
 float4 w0 : WORLD0; float4 w1 : WORLD1; float4 w2 : WORLD2; float4 w3 : WORLD3;
 float4 n0 : INORMAL0; float4 n1 : INORMAL1; float4 n2 : INORMAL2; float4 n3 : INORMAL3;
 float4 paint : PAINT; float4 surface : SURFACE;
};
struct VOut {
 float4 p : SV_POSITION; float3 n : NORMAL; float3 wp : TEXCOORD0;
 float4 tint : COLOR; float4 shadow : TEXCOORD1; nointerpolation float material : TEXCOORD2;
};
VOut VSMain(VIn i) {
 VOut o; o.p=mul(float4(i.p,1),wvp); o.wp=i.n; o.n=i.n;
 o.tint=i.tint; o.shadow=0; o.material=0; return o;
}
VOut VSWorld(InstanceIn i) {
 VOut o;
 float4x4 object=float4x4(i.w0,i.w1,i.w2,i.w3);
 float4x4 normals=float4x4(i.n0,i.n1,i.n2,i.n3);
 float4 position=mul(float4(i.p,1),object);
 o.wp=position.xyz; o.p=mul(position,wvp);
 o.n=normalize(mul(float4(i.n,0),normals).xyz);
 o.tint=i.tint*i.paint; o.shadow=mul(position,shadowVP); o.material=i.surface.x;
 return o;
}
float Hash(float2 p) { float3 q=frac(float3(p.xyx)*.1031); q+=dot(q,q.yzx+33.33); return frac((q.x+q.y)*q.z); }
float Noise(float2 p) {
 float2 cell=floor(p),f=frac(p); f=f*f*(3-2*f);
 return lerp(lerp(Hash(cell),Hash(cell+float2(1,0)),f.x),lerp(Hash(cell+float2(0,1)),Hash(cell+1),f.x),f.y);
}
float Shadow(float4 position,float3 n) {
 float3 projected=position.xyz/position.w;
 float2 uv=projected.xy*float2(.5,-.5)+.5;
 if (any(uv<0) || any(uv>1) || projected.z<0 || projected.z>1) return 1;
 float bias=max(.0005,.0015*(1-saturate(dot(n,normalize(float3(-.5,.85,-.4))))));
 float sum=0;
 uint width,height; shadowMap.GetDimensions(width,height);
 [unroll] for (int y=-1;y<=1;++y) [unroll] for (int x=-1;x<=1;++x)
  sum+=shadowMap.SampleCmpLevelZero(shadowSampler,uv+float2(x,y)/float2(width,height),projected.z-bias);
 return sum/9;
}
float4 PSMain(VOut i) : SV_TARGET {
 if (eyeAndHud.w>.5) {
  float4 result=color*i.tint;
  if (i.wp.z>1.5) result*=mapArtwork.Sample(fontSampler,i.wp.xy);
  else if (i.wp.z>.5) result.a*=fontAtlas.Sample(fontSampler,i.wp.xy);
  return result;
 }
 float3 n=normalize(i.n),absolute=abs(n);
 float2 uv=absolute.y>max(absolute.x,absolute.z)?i.wp.xz: absolute.x>absolute.z?i.wp.zy:i.wp.xy;
 float scale=i.material==3?.8:i.material==5?1.1:i.material==6?3: .6;
 float detail=materials.Sample(surfaceSampler,float3(uv*scale,i.material)).r;
 float3 albedo=i.tint.rgb*detail;
 if (i.material==1) {
  float broad=Noise(i.wp.xz*.16),fine=Noise(i.wp.xz*1.7);
  albedo=lerp(float3(.19,.28,.105),float3(.36,.42,.19),broad)*detail;
  // The 256-square weight map shares authored routes with the atlas and cover.
  float2 groundUV=float2((i.wp.x+21)/42,(23-i.wp.z)/46);
  float2 weights=terrainSurface.Sample(fontSampler,groundUV).rg;
  weights*=all(groundUV>=0) && all(groundUV<=1);
  float3 soil=float3(.40,.32,.22)*(.83+fine*.2)*detail;
  float2 tile=i.wp.xz*float2(2.2,3.3);
  tile.x+=floor(tile.y)*.5;
  float2 cell=floor(tile),fraction=frac(tile);
  float joint=min(min(fraction.x,1-fraction.x),min(fraction.y,1-fraction.y));
  float smoothing=max(max(fwidth(tile.x),fwidth(tile.y)),.012);
  float mortar=smoothstep(.035-smoothing,.075+smoothing,joint);
  float3 paving=lerp(float3(.21,.22,.20),float3(.48,.46,.39)*(.86+Hash(cell)*.22),mortar)*detail;
  albedo=albedo*max(0,1-weights.x-weights.y)+soil*weights.x+paving*weights.y;
 }
 float3 lightDir=normalize(eyeAndHud.w<0?float3(-.3,.6,1):float3(-.5,.85,-.4));
 float diffuse=saturate(dot(n,lightDir));
 float visibility=eyeAndHud.w<0?1:Shadow(i.shadow,n);
 float hemisphere=lerp(.19,.48,n.y*.5+.5);
 // A small transmitted-light term keeps the backs of opaque leaves readable.
 if (i.material==8) { hemisphere=max(hemisphere,.40); diffuse+=saturate(dot(-n,lightDir))*.25; }
 float3 light=float3(.72,.79,.91)*hemisphere+float3(1.0,.90,.73)*diffuse*visibility*.8;
 float3 result=albedo*light;
 if (i.material==7) {
  float3 halfVector=normalize(lightDir+normalize(eyeAndHud.xyz-i.wp));
  result+=pow(saturate(dot(n,halfVector)),40)*visibility*.32;
 }
 if (i.material==10) result=albedo*1.25;
 float fog=1-exp(-max(0,length(i.wp-eyeAndHud.xyz)-18)*.012);
 result=lerp(result,float3(.43,.51,.57),fog);
 return float4(result,1);
}
struct SkyOut { float4 p : SV_POSITION; float2 uv : TEXCOORD0; };
SkyOut VSSky(uint id : SV_VertexID) {
 SkyOut o; o.uv=float2((id<<1)&2,id&2); o.p=float4(o.uv*float2(2,-2)+float2(-1,1),.9999,1); return o;
}
float4 PSSky(SkyOut i) : SV_TARGET {
 float h=saturate(1-i.uv.y);
 float3 sky=lerp(float3(.54,.59,.63),float3(.16,.29,.43),h);
 float clouds=Noise(i.uv*float2(10,7))+Noise(i.uv*float2(24,16))*.35;
 float haze=smoothstep(.7,1.1,clouds)*smoothstep(.15,.65,h)*.22;
 return float4(sky+haze,1);
}
)";
