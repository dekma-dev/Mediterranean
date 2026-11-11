R"(
// A small local ray integral: no captured image, noise texture, screen copy or
// scene-sized offscreen target. The conservative front proxy is depth-clipped
// against opaque world geometry inside the volume, not at its centre plane.
float SpellNoise(float3 p) {
 float z=floor(p.z),f=frac(p.z);f=f*f*(3-2*f);
 return lerp(Noise(p.xy+float2(z*37,z*17)),Noise(p.xy+float2((z+1)*37,(z+1)*17)),f);
}
float3 SpellLocal(SpellVolume v,float3 p) {
 return float3(dot(p,v.rightRadius.xyz)/v.rightRadius.w,
               dot(p,v.upRadius.xyz)/v.upRadius.w,dot(p,v.axisRadius.xyz)/v.axisRadius.w);
}
float4 SpellRadiance(EffectOut i) {
 uint index=min(23u,(uint)round(i.uv.z)-32u);
 SpellVolume v=spellVolumes[index];
 float3 worldRay=i.wp-eyeAndHud.xyz;
 float rayLength=length(worldRay);worldRay/=max(rayLength,.0001);
 float3 ro=SpellLocal(v,eyeAndHud.xyz-v.centerKind.xyz),rd=SpellLocal(v,worldRay);
 float a=dot(rd,rd),b=dot(ro,rd),c=dot(ro,ro)-1,discriminant=b*b-a*c;
 if(discriminant<=0) return 0;
 float halfSpan=sqrt(discriminant),nearT=max(0,(-b-halfSpan)/a),farT=(-b+halfSpan)/a;
 float z=effectSceneDepth.Load(int3((int2)i.p.xy,0)),farPlane=eyeAndHud.w<0?20:280;
 float sceneViewDepth=(.1*farPlane)/max(.0001,farPlane-z*(farPlane-.1));
 float sceneRayDepth=sceneViewDepth*rayLength/max(.0001,i.viewDepth);
 farT=min(farT,sceneRayDepth);
 if(farT<=nearT) return 0;
 uint steps=(uint)v.clock.z;
 float stepSize=(farT-nearT)/steps;
 float normalizedStep=stepSize*pow(a,.5);
 float phase=v.clock.x,seed=v.clock.y*.071;
 float3 energy=0;float transmission=1;
 // Fixed spatial quadrature avoids frame-random speckle and preserves exact
 // equal-age results. Small projected volumes use 12 steps; larger ones 20.
 [loop] for(uint j=0;j<steps;j++) {
  float t=nearT+(j+.5)*stepSize;
  float3 p=ro+rd*t;
  float boundary=smoothstep(0,.20,1-dot(p,p));
  float3 flow=p*3.4+float3(seed,-phase*.35,phase*3.8);
  float n=SpellNoise(flow),fine=SpellNoise(flow*2.1+float3(phase*.6,11,7));
  float turbulence=n*.72+fine*.28;
  float density=0;float3 emission=0;
  if(v.centerKind.w<.5) {
   float radius=length(p.xy+float2(n-.5,fine-.5)*.18);
   float envelope=saturate(1-radius*1.24-abs(p.z)*.24);
   density=saturate(envelope*1.4+(turbulence-.55)*3.5)*boundary;
   float hot=saturate(1.1-radius*1.5-abs(p.z)*.25+(turbulence-.5)*.9);
   emission=lerp(float3(1,.065,.008),float3(1,.46,.035),smoothstep(.12,.65,hot));
   emission=lerp(emission,float3(1,.90,.55),smoothstep(.70,.98,hot));
  } else if(v.centerKind.w<1.5) {
   float crystal=max(abs(p.x)*.866+abs(p.y)*.5,abs(p.y));
   float shell=pow(saturate(1-abs(crystal-(.36-.17*p.z))*9),3);
   density=(shell*.8+saturate(turbulence-.51)*.9)*boundary;
   emission=lerp(float3(.08,.30,.75),float3(.55,.88,1),saturate(shell+turbulence*.3));
  } else if(v.centerKind.w<2.5) {
   float radius=length(p.xy),twist=atan2(p.y,p.x)*3+p.z*8-phase*8;
   float filament=pow(saturate(.5+.5*sin(twist)),5);
   float shell=saturate(1-abs(radius-(.42+.10*sin(atan2(p.y,p.x)*3+phase*4)+.08*sin(p.z*8-phase*5)))*5);
   density=(shell*(.18+filament*.75)+saturate(.20-radius)*2+saturate(turbulence-.55)*.3)*boundary;
   emission=lerp(float3(.29,.055,.85),float3(.68,.78,1),filament*.72);
  } else {
   float radius=length(p.xy),core=pow(saturate(1-radius*2.2),3);
   float filaments=pow(saturate(.5+.5*sin(atan2(p.y,p.x)*6+p.z*5-phase*4)),8);
   density=(core*.8+filaments*.28*saturate(1-radius)+saturate(turbulence-.56)*.55)*boundary;
   emission=lerp(float3(1,.35,.035),float3(1,.95,.72),saturate(core*2+filaments*.4));
  }
  // Extinction leaves an opaque hot heart and a low-density rolling boundary.
  // Soft surface contact is evaluated at every sample, including walls passing
  // through the middle of a volume and a view nearly parallel to its axis.
  density*=saturate((sceneRayDepth-t)/.06);
  float opacity=1-exp(-density*normalizedStep*2.5);
  energy+=transmission*opacity*emission;
  transmission*=1-opacity;
 }
 float alpha=1-transmission;
 return float4(saturate(energy/max(alpha,.001)*v.tint.rgb*1.10),alpha*saturate(v.tint.w)*i.fog);
}
)"
