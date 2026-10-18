#pragma once
inline constexpr char sceneShader[] = R"(
cbuffer Scene : register(b0) {
 matrix world; matrix wvp; matrix normalWorld; matrix shadowVP[2];
 float4 color; float4 eyeAndHud;
};
Texture2DArray materials : register(t0);
Texture2DArray<float> shadowMap : register(t1);
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
 float4 detailShadow : TEXCOORD3;
 float3 tp : TEXCOORD4; float3 tn : TEXCOORD5;
};
VOut VSMain(VIn i) {
 VOut o; o.p=mul(float4(i.p,1),wvp); o.wp=i.n; o.n=i.n;
 o.tint=i.tint; o.shadow=o.detailShadow=0; o.material=0; o.tp=o.tn=0; return o;
}
float4 VSShadow(InstanceIn i) : SV_POSITION {
 float4x4 object=float4x4(i.w0,i.w1,i.w2,i.w3);
 return mul(mul(float4(i.p,1),object),wvp);
}
VOut VSWorld(InstanceIn i) {
 VOut o;
 float4x4 object=float4x4(i.w0,i.w1,i.w2,i.w3);
 float4x4 normals=float4x4(i.n0,i.n1,i.n2,i.n3);
 float4 position=mul(float4(i.p,1),object);
 o.wp=position.xyz; o.p=mul(position,wvp);
 o.n=normalize(mul(float4(i.n,0),normals).xyz);
 o.tint=i.tint*i.paint; o.shadow=mul(position,shadowVP[0]);
 o.detailShadow=mul(position,shadowVP[1]); o.material=i.surface.x;
 // Metric object coordinates follow animated parts instead of sliding through
 // a world-space texture. Projection weights account for nonuniform scaling.
 float3 size=float3(length(i.w0.xyz),length(i.w1.xyz),length(i.w2.xyz));
 o.tp=i.p*size; o.tn=normalize(i.n/max(size,.00001));
 return o;
}
float Hash(float2 p) { float3 q=frac(float3(p.xyx)*.1031); q+=dot(q,q.yzx+33.33); return frac((q.x+q.y)*q.z); }
float Noise(float2 p) {
 float2 cell=floor(p),f=frac(p); f=f*f*(3-2*f);
 return lerp(lerp(Hash(cell),Hash(cell+float2(1,0)),f.x),lerp(Hash(cell+float2(0,1)),Hash(cell+1),f.x),f.y);
}
float3 ShadowCoordinate(float4 position) {
 float3 projected=position.xyz/position.w;
 float2 uv=projected.xy*float2(.5,-.5)+.5;
 return float3(uv,projected.z);
}
float2 ShadowGradient(float3 coordinate) {
 // Derivatives must be evaluated before the per-pixel coverage branch. Fit the
 // receiver plane in shadow UV space so adjacent PCF taps compare at its depth,
 // instead of compensating for every slope with a detached, oversized bias.
 float3 dx=ddx(coordinate),dy=ddy(coordinate);
 float determinant=dx.x*dy.y-dx.y*dy.x;
 float2 gradient=0;
 if (abs(determinant)>max(length(dx.xy)*length(dy.xy),1e-20)*.0001)
  gradient=float2(dy.y*dx.z-dx.y*dy.z,dx.x*dy.z-dy.x*dx.z)/determinant;
 return gradient;
}
float4 ShadowWeights(float fraction) {
 // Cubic B-spline weights: continuous first/second derivatives at texel
 // boundaries, with a tighter central lobe than the old linear tent.
 float f2=fraction*fraction,f3=f2*fraction;
 float inv=1-fraction;
 return float4(inv*inv*inv,3*f3-6*f2+4,-3*f3+3*f2+3*fraction+1,f3)/6;
}
float ShadowFilter(float3 coordinate,float2 gradient,float sunFacing,uint layer) {
 float2 uv=coordinate.xy;
 if (any(uv<0) || any(uv>1) || coordinate.z<0 || coordinate.z>1) return 1;
 uint width,height,layers; shadowMap.GetDimensions(width,height,layers);
 float2 size=float2(width,height);
 // Bound ill-conditioned grazing planes. Compare at each actual texel center:
 // one comparison depth for a hardware bilinear footprint produces scalloped
 // edges on wall trim, even with a receiver-plane correction between taps.
 // Keep the larger valid slope range exposed by linear-light shading at dawn
 // angles, while still bounding near-singular receiver planes.
 float2 depthStep=clamp(gradient/size,-.02,.02);
 float2 texel=uv*size-.5,base=floor(texel),fraction=texel-base;
 float grazing=1-sunFacing;
 float bias=.000025+.00015*grazing*grazing;
 // Sixteen point comparisons retain a separate receiver-plane depth per tap.
 // Cubic weights reduce filtering-induced speed changes as edges cross texels.
 float4 wx=ShadowWeights(fraction.x),wy=ShadowWeights(fraction.y);
 float sum=0;
 [unroll] for (int y=0;y<4;++y) [unroll] for (int x=0;x<4;++x) {
  float2 tap=base+float2(x-1,y-1);
  sum+=wx[x]*wy[y]*shadowMap.SampleCmpLevelZero(shadowSampler,float3((tap+.5)/size,layer),
    saturate(coordinate.z+dot(depthStep,tap-texel)-bias));
 }
 return sum;
}
float Shadow(float4 widePosition,float4 detailPosition,float sunFacing) {
 float3 wide=ShadowCoordinate(widePosition),detail=ShadowCoordinate(detailPosition);
 // Compute both receiver planes before any cascade-dependent branch; derivatives
 // inside a divergent selection can introduce stripes at the blend boundary.
 float2 wideGradient=ShadowGradient(wide),detailGradient=ShadowGradient(detail);
 float margin=min(min(detail.x,1-detail.x),min(detail.y,1-detail.y));
 float weight=smoothstep(.03,.15,margin);
 if (detail.z<0 || detail.z>1) weight=0;
 float nearValue=1,farValue=1;
 [branch] if (weight>0) nearValue=ShadowFilter(detail,detailGradient,sunFacing,1);
 [branch] if (weight<1) farValue=ShadowFilter(wide,wideGradient,sunFacing,0);
 return lerp(farValue,nearValue,weight);
}
float4 PSMain(VOut i) : SV_TARGET {
 if (eyeAndHud.w>.5) {
  float4 result=color*i.tint;
  if (i.wp.z>1.5) result*=mapArtwork.Sample(fontSampler,i.wp.xy);
  else if (i.wp.z>.5) result.a*=fontAtlas.Sample(fontSampler,i.wp.xy);
  return result;
 }
 float3 geometric=normalize(i.n),n=geometric;
 float3 weights=pow(abs(normalize(i.tn)),4); weights/=max(dot(weights,1),.00001);
 float scale=i.material==3?.8:i.material==5?1.1:i.material==6?3: .6;
 float4 mx=materials.Sample(surfaceSampler,float3(i.tp.zy*scale,i.material));
 float4 my=materials.Sample(surfaceSampler,float3(i.tp.xz*scale,i.material));
 float4 mz=materials.Sample(surfaceSampler,float3(i.tp.xy*scale,i.material));
 float4 surface=mx*weights.x+my*weights.y+mz*weights.z;
 float detail=surface.r,roughness=surface.a;
 float2 gx=(mx.gb*255-128)/32,gy=(my.gb*255-128)/32,gz=(mz.gb*255-128)/32;
 float3 slope=weights.x*float3(0,gx.y,gx.x)+weights.y*float3(gy.x,0,gy.y)+weights.z*float3(gz.x,gz.y,0);
 float2 heightDerivative=float2(dot(slope,ddx(i.tp)),dot(slope,ddy(i.tp)))*scale;
 float3 albedo=i.tint.rgb*detail;
 float cavity=1;
 if (i.material==1) {
  float broad=Noise(i.wp.xz*.16),fine=Noise(i.wp.xz*1.7);
  albedo=lerp(float3(.22,.32,.12),float3(.39,.43,.20),broad)*detail;
  // The 256-square weight map shares authored routes with the atlas and cover.
  float2 groundUV=float2((i.wp.x+21)/42,(23-i.wp.z)/46);
  float2 routes=terrainSurface.Sample(fontSampler,groundUV).rg;
  routes*=all(groundUV>=0) && all(groundUV<=1);
  float3 soil=float3(.43,.34,.23)*(.83+fine*.2)*detail;
  float2 tile=i.wp.xz*float2(2.2,3.3);
  tile+=.10*sin(tile.yx*float2(2.9,3.7));
  tile.x+=floor(tile.y)*.5;
  float2 cell=floor(tile),fraction=frac(tile);
  // Rounded, chipped corners keep the path from reading as a perfect brick grid.
  float2 edgeDistance=abs(fraction-.5);
  float joint=.5-pow(pow(edgeDistance.x,6)+pow(edgeDistance.y,6),1.0/6);
  float chips=Noise(i.wp.xz*17);
  joint-=(chips-.5)*.055;
  float smoothing=max(max(fwidth(tile.x),fwidth(tile.y)),.012);
  float mortar=smoothstep(.035-smoothing,.075+smoothing,joint);
  float stoneVariation=Hash(cell);
  float3 stoneColor=lerp(float3(.37,.40,.42),float3(.59,.53,.43),stoneVariation);
  float moss=(1-mortar)*smoothstep(.35,.7,fine);
  float3 jointColor=lerp(float3(.19,.18,.15),float3(.19,.25,.12),moss);
  float3 paving=lerp(jointColor,stoneColor*(.92+.12*chips),mortar)*detail;
  albedo=albedo*max(0,1-routes.x-routes.y)+soil*routes.x+paving*routes.y;
  // Shallow stone bevels shade the paving without changing the collision floor.
  float relief=routes.y*(.042*mortar*(.7+stoneVariation*.3)+.002*chips*mortar);
  heightDerivative+=float2(ddx(relief),ddy(relief));
  roughness=lerp(.96,lerp(.93,.68+.14*stoneVariation,mortar),routes.y);
  cavity=lerp(1,lerp(.65,1,mortar),routes.y);
 }
 if (i.material==4) {
  // Broad limewash variation and rain streaks on static plaster walls. Use
  // both horizontal axes so weathering continues around building corners.
  float wash=Noise(i.wp.xz*.65+i.wp.y*.12);
  float streak=Noise(float2((i.wp.x+i.wp.z)*5.3,i.wp.y*.32));
  float stain=(1-smoothstep(.12,1.65,i.wp.y))*(.35+.65*streak);
  albedo*=lerp(.89,1.035,wash);
  albedo=lerp(albedo,albedo*float3(.63,.66,.53),stain*.65);
  albedo*=1-.065*smoothstep(.55,.85,streak);
 }
 // Convert the texture height derivative into a world-space surface gradient.
 // This works for rotated/nonuniform parts without stored tangent vertices.
 float3 dpdx=ddx(i.wp),dpdy=ddy(i.wp);
 float3 rx=cross(dpdy,n),ry=cross(n,dpdx);
 float determinant=dot(dpdx,rx);
 if (abs(determinant)>1e-12) {
  float3 gradient=(rx*heightDerivative.x+ry*heightDerivative.y)/determinant;
  gradient*=min(1,.7/max(length(gradient),.00001));
  n=normalize(n-gradient);
 }
 float3 lightDir=normalize(eyeAndHud.w<0?float3(-.3,.6,1):float3(-.5,.85,-.4));
 float diffuse=saturate(dot(n,lightDir));
 float visibility=eyeAndHud.w<0?1:Shadow(i.shadow,i.detailShadow,saturate(dot(geometric,lightDir)));
 float hemisphere=lerp(.20,.58,n.y*.5+.5);
 // A small transmitted-light term keeps the backs of opaque leaves readable.
 if (i.material==8) { hemisphere=max(hemisphere,.48); diffuse+=saturate(dot(-n,lightDir))*.32; }
 float3 base=pow(saturate(albedo),2.2);
 float metal=i.material==7?.8:0;
 float3 view=normalize(eyeAndHud.xyz-i.wp),halfVector=normalize(lightDir+view);
 float exponent=lerp(96,5,roughness*roughness);
 float3 fresnel=lerp(.035,base,metal);
 fresnel+=(1-fresnel)*pow(1-saturate(dot(view,halfVector)),5);
 float3 ambient=lerp(float3(.29,.23,.16),float3(.52,.68,.91),n.y*.5+.5)*hemisphere;
 float3 sunlight=float3(1,.88,.70)*2.15;
 float specular=pow(saturate(dot(n,halfVector)),exponent)*(exponent+8)/32;
 float3 result=base*(ambient*cavity+sunlight*diffuse*visibility)*(1-metal*.45);
 result+=fresnel*(specular*sunlight*visibility*diffuse+ambient*.45);
 if (i.material==10) result=base*2.3;
 // Resolve lighting in linear space, then apply a restrained highlight shoulder.
 result=pow(max(0,result/(1+result)),1/2.2);
 float fog=1-exp(-max(0,length(i.wp-eyeAndHud.xyz)-18)*.012);
 result=lerp(result,float3(.43,.51,.57),fog);
 return float4(result,1);
}
struct EffectOut { float4 p : SV_POSITION; float3 uv : TEXCOORD0; float4 tint : COLOR; float fog : TEXCOORD1; };
EffectOut VSEffect(VIn i) {
 EffectOut o; o.p=mul(float4(i.p,1),wvp); o.uv=i.n; o.tint=i.tint;
 o.fog=exp(-max(0,length(i.p-eyeAndHud.xyz)-18)*.012); return o;
}
float4 PSEffect(EffectOut i) : SV_TARGET {
 float2 p=i.uv.xy*2-1;
 float radius=length(p),edge=saturate(1-abs(p.y)),mask=0,core=0;
 if (i.uv.z<.5) {
  core=pow(edge,8); mask=.60*edge*edge+.40*pow(edge,4);
 } else if (i.uv.z<1.5) {
  core=pow(saturate(1-dot(p,p)),16);
  mask=.28*pow(saturate(1-dot(p,p)),2)+.72*core;
 } else if (i.uv.z<2.5) {
  // Analytic shock ring, with a narrow core and a wider halo. Derivative-aware
  // width keeps the edge stable when the impact is small or seen at a distance.
  float distance=abs(radius-.72),width=max(.035,fwidth(radius));
  core=1-smoothstep(width,width*2,distance);
  mask=core*.72+pow(saturate(1-distance/.22),2)*.28;
 } else {
  float ends=pow(saturate(1-p.x*p.x),2);
  core=pow(edge,8)*ends; mask=(.28*edge*edge+.72*pow(edge,4))*ends;
 }
 float3 luminous=lerp(i.tint.rgb,float3(1,.98,.92),core*(i.uv.z<.5?.42:.72));
 return float4(luminous,saturate(i.tint.a)*mask*i.fog);
}
struct SkyOut { float4 p : SV_POSITION; float2 uv : TEXCOORD0; };
SkyOut VSSky(uint id : SV_VertexID) {
 SkyOut o; o.uv=float2((id<<1)&2,id&2); o.p=float4(o.uv*float2(2,-2)+float2(-1,1),.9999,1); return o;
}
float4 PSSky(SkyOut i) : SV_TARGET {
 // The sky pass uses the existing world matrix slot for inverse view-projection.
 // Direction changes with orbit/pitch/aspect, but ignores camera translation.
 float4 farPosition=mul(float4(i.uv*float2(2,-2)+float2(-1,1),1,1),world);
 float3 ray=normalize(farPosition.xyz/farPosition.w-eyeAndHud.xyz);
 float h=saturate(ray.y*.65+.35);
 float3 sky=lerp(float3(.69,.74,.76),float3(.22,.39,.58),pow(h,.65));
 // Direction-domain noise avoids a longitude seam and horizon division spikes.
 float2 p=ray.xz*3.5+ray.y*float2(1.3,-1.7);
 float broad=Noise(p),clouds=broad*.55+Noise(p*2.1)*.26+Noise(p*4.3)*.13+Noise(p*8.1)*.06;
 float cover=smoothstep(.48,.73,clouds)*smoothstep(.08,.5,h);
 float3 cloudColor=lerp(float3(.59,.66,.73),float3(.94,.92,.85),saturate(broad*.7+clouds*.5));
 return float4(lerp(sky,cloudColor,cover*.82),1);
}
)";
