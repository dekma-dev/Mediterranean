R"(
// Only accepted, visible combat emission enters this target. The sky, scene
// materials, UI artwork and text never go through the glare filter.
cbuffer SpellGlow : register(b6) {
 float4 glowTexel; // inverse allocation dimensions, allocation dimensions
 float4 glowScreen; // low-resolution raster -> original depth pixel
 float4 glowRegion; // normalized current world/portrait viewport
};
Texture2D<float4> spellGlowSource : register(t16);
float4 PSGlowSource(EffectOut i) : SV_TARGET {
 uint kind=(uint)round(i.uv.z);
 i.p.xy*=glowScreen.xy;
 float4 c=EffectColor(i);
 if(kind==9 || kind==10) return 0; // smoke and dust scatter; they do not emit
 float farPlane=eyeAndHud.w<0?20:280;
 float z=effectSceneDepth.Load(int3((int2)i.p.xy,0));
 float sceneDepth=(.1*farPlane)/max(.0001,farPlane-z*(farPlane-.1));
 // The reduced target has no DSV. Perform the ordinary geometry visibility
 // test here; media instead integrate their own opaque interior clipping.
 if(kind<32 && i.viewDepth>sceneDepth+.005) return 0;
 float peak=max(c.r,max(c.g,c.b));
 float knee=smoothstep(.46,.92,peak);
 return float4(max(0,c.rgb)*saturate(c.a)*knee,1);
}
struct GlowOut {float4 p:SV_POSITION;float2 uv:TEXCOORD0;};
GlowOut VSGlow(uint id:SV_VertexID) {
 GlowOut o;float2 uv=float2((id<<1)&2,id&2);
 o.p=float4(uv*float2(2,-2)+float2(-1,1),0,1);o.uv=uv;return o;
}
float3 GlowSample(float2 uv) {
 if(any(uv<glowRegion.xy) || any(uv>glowRegion.zw))return 0;
 return spellGlowSource.SampleLevel(fontSampler,uv,0).rgb;
}
float4 GlowBlur(float2 uv,float2 axis) {
 float3 energy=GlowSample(uv)*.227027;
 energy+=(GlowSample(uv+axis*1.384615)+GlowSample(uv-axis*1.384615))*.316216;
 energy+=(GlowSample(uv+axis*3.230769)+GlowSample(uv-axis*3.230769))*.070270;
 return float4(energy,1);
}
float4 PSGlowHorizontal(GlowOut i):SV_TARGET {return GlowBlur(i.uv,float2(glowTexel.x*2.6,0));}
float4 PSGlowVertical(GlowOut i):SV_TARGET {return GlowBlur(i.uv,float2(0,glowTexel.y*2.6));}
float4 PSGlowComposite(GlowOut i):SV_TARGET {
 if(any(i.uv<glowRegion.xy) || any(i.uv>glowRegion.zw))return 0;
 float3 energy=GlowSample(i.uv);
 // Bounded, hue-preserving radiance. A crowded impact must not paint the
 // whole viewport white; its sharp core is already present in the main pass.
 return float4(energy*1.35/(1+1.9*max(energy.r,max(energy.g,energy.b))),1);
}
)"
