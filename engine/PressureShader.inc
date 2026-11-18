R"(
// Scene refraction precedes every warning/effect/HUD batch. No colour tint or
// feedback accumulation: a frame samples exactly one immutable scene snapshot.
struct PressureWave {float4 centerStrength;float4 radiiAge;float4 axisStyle;};
cbuffer PressureScene : register(b8) {
 matrix pressureInverseVP;matrix pressureVP;
 float4 pressureEye;float4 pressureScreen;float4 pressureRegion;
 PressureWave pressureWaves[8];
};
Texture2D<float4> pressureSceneColor : register(t18);
float3 PressureWorld(float2 uv,float depth){
 float4 p=mul(float4(uv*float2(2,-2)+float2(-1,1),depth,1),pressureInverseVP);
 return p.xyz/max(p.w,.000001);
}
float2 PressureProject(float3 p){
 float4 clip=mul(float4(p,1),pressureVP);
 return clip.xy/max(.000001,clip.w)*float2(.5,-.5)+.5;
}
float3 PressureLocal(float3 p,float2 sc){return float3(p.x*sc.y-p.z*sc.x,p.y,p.x*sc.x+p.z*sc.y);}
float3 PressureGlobal(float3 p,float2 sc){return float3(p.x*sc.y+p.z*sc.x,p.y,-p.x*sc.x+p.z*sc.y);}
float4 PSPressure(GlowOut i):SV_TARGET {
 float2 uv=i.p.xy*pressureScreen.xy;
 float4 original=pressureSceneColor.Load(int3((int2)i.p.xy,0));
 float depth=effectSceneDepth.Load(int3((int2)i.p.xy,0));
 float3 world=PressureWorld(uv,depth),ray=normalize(world-pressureEye.xyz);
 float distance=length(world-pressureEye.xyz);float2 displacement=0;
 float nearest=1e10;
 [loop]for(uint n=0;n<(uint)pressureEye.w;++n){
  PressureWave wave=pressureWaves[n];float3 radii=wave.radiiAge.xyz;
  float3 origin=PressureLocal(pressureEye.xyz-wave.centerStrength.xyz,wave.axisStyle.xy)/radii;
  float3 direction=PressureLocal(ray,wave.axisStyle.xy)/radii;
  float a=dot(direction,direction),b=dot(origin,direction),c=dot(origin,origin)-1;
  float discriminant=b*b-a*c;if(discriminant<=0)continue;
  float entry=(-b-sqrt(discriminant))/a,exit=(-b+sqrt(discriminant))/a;
  if(exit<=0 || entry>=distance-.015)continue;
  float front=max(0,entry);float3 local=origin+direction*front;
  float radial=sqrt(saturate(dot(origin,origin)-b*b/a));
  float rim=exp(-pow((radial-.86)/.115,2));
  float visibility=saturate((distance-front)/.20);
  float3 normal=normalize(PressureGlobal(local/radii,wave.axisStyle.xy));
  float3 at=pressureEye.xyz+ray*front;
  float2 screenNormal=(PressureProject(at+normal*.1)-uv)/pressureScreen.xy;
  screenNormal/=max(1,length(screenNormal));
  float noise=sin(local.x*17+local.z*13-wave.radiiAge.w*24)*sin(local.y*19+wave.radiiAge.w*17);
  float ripple=wave.axisStyle.z==1?(.78+noise*.22):wave.axisStyle.z==2?(1+noise*.12):(.9+noise*.1);
  displacement+=screenNormal*rim*visibility*wave.centerStrength.w*ripple*pressureScreen.z;
  if(rim*visibility>.001)nearest=min(nearest,front);
 }
 float amplitude=length(displacement);displacement*=min(1,pressureScreen.z/max(.0001,amplitude));
 if(amplitude<.001)return original;
 float2 shifted=clamp(uv+displacement*pressureScreen.xy,pressureRegion.xy,pressureRegion.zw);
 float sampleZ=effectSceneDepth.Load(int3((int2)(shifted/pressureScreen.xy),0));
 float3 sampleWorld=PressureWorld(shifted,sampleZ);float sampleDistance=length(sampleWorld-pressureEye.xyz);
 // Don't pull foreground silhouettes into an effect behind them or stretch
 // an opaque depth discontinuity into a background halo.
 if(sampleDistance<nearest+.015 || abs(sampleDistance-distance)>max(.18,distance*.025))return original;
 return float4(pressureSceneColor.SampleLevel(fontSampler,shifted,0).rgb,original.a);
}
)"
