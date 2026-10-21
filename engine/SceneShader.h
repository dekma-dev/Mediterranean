#pragma once
// Keep each raw literal below MSVC's 16 KiB token limit. Adjacent literals
// concatenate into the same shader source, without runtime loading or copies.
inline constexpr char sceneShader[] = R"(
cbuffer Scene : register(b0) {
 matrix world; matrix wvp; matrix normalWorld; matrix shadowVP[2];
 float4 color; float4 eyeAndHud;
 float4 sun; float4 atmosphere;
};
Texture2DArray materials : register(t0);
Texture2DArray<float> shadowMap : register(t1);
Texture2D<float> fontAtlas : register(t2);
Texture2D<float4> mapArtwork : register(t3);
Texture2D<float4> terrainSurface : register(t4);
Texture2D<float4> uiFrame : register(t5);
Texture2D<float4> uiHall : register(t6);
Texture2D<float4> uiIcons : register(t7);
Texture2D<float> cloudLightField : register(t8);
// x = world half extent, y = last texel-center world coordinate. A zero
// extent selects the analytic reference path for regression diagnostics.
cbuffer CloudField : register(b3) { float4 cloudField; };
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
 float cloudLight : TEXCOORD6;
 nointerpolation float receiverPlane : TEXCOORD7;
};
float CloudSunlight(float3 position);
float SurfaceCloudSunlight(float3 position);
VOut VSMain(VIn i) {
 VOut o; o.p=mul(float4(i.p,1),wvp); o.wp=i.n; o.n=i.n;
 o.tint=i.tint; o.shadow=o.detailShadow=0; o.material=0; o.tp=o.tn=0; o.cloudLight=1; o.receiverPlane=1; return o;
}
void DeformCloth(inout float3 p,inout float3 n,float4 surface) {
 [branch] if (surface.w>0) {
  float t=saturate(-p.y/.82),side=surface.z,back=surface.w-.001;
  p.x+=side*t*t; p.z-=back*t*t;
  // Inverse transpose of the quadratic bend; no extra vertex buffers.
  n.y+=2*t/.82*(side*n.x-back*n.z);
 }
}
float4 VSShadow(InstanceIn i) : SV_POSITION {
 float4x4 object=float4x4(i.w0,i.w1,i.w2,i.w3);
 DeformCloth(i.p,i.n,i.surface);
 return mul(mul(float4(i.p,1),object),wvp);
}
VOut VSWorld(InstanceIn i) {
 VOut o;
 float4x4 object=float4x4(i.w0,i.w1,i.w2,i.w3);
 float4x4 normals=float4x4(i.n0,i.n1,i.n2,i.n3);
 float3 undeformed=i.p,undeformedNormal=i.n;
 DeformCloth(i.p,i.n,i.surface);
 float4 position=mul(float4(i.p,1),object);
 o.wp=position.xyz; o.p=mul(position,wvp);
 o.n=normalize(mul(float4(i.n,0),normals).xyz);
 o.receiverPlane=i.surface.w<0?0:1;
 float4 receiver=position;
 if (i.surface.w<0) receiver.xyz+=o.n*(i.surface.w<-1?.0007:.0025);
 o.tint=i.tint*i.paint; o.shadow=mul(receiver,shadowVP[0]);
 o.detailShadow=mul(receiver,shadowVP[1]); o.material=i.surface.x;
 // Metric object coordinates follow animated parts instead of sliding through
 // a world-space texture. Projection weights account for nonuniform scaling.
 float3 size=float3(length(i.w0.xyz),length(i.w1.xyz),length(i.w2.xyz));
 o.tp=undeformed*size; o.tn=normalize(undeformedNormal/max(size,.00001));
 // The cloud footprint spans hundreds of metres. Interpolating this slow field
 // avoids a volume integration in every opaque pixel, including overdraw.
 o.cloudLight=1;
 [branch] if (eyeAndHud.w>=0) o.cloudLight=SurfaceCloudSunlight(position.xyz);
 return o;
}
)" R"(
// A shared palette is reused by color, both shadow cascades and portraits.
struct BodyPalette { matrix joints[10]; matrix normals[10]; float4 colors[6]; };
cbuffer Bodies : register(b1) { BodyPalette bodies[32]; };
struct SkinIn { InstanceIn object; uint4 bones : BLENDINDICES; float4 weights : BLENDWEIGHT; };
void SkinBody(inout InstanceIn i,uint4 joints,float4 weights) {
 uint actor=(uint)i.surface.z;
 float3 p=0,n=0;
 [unroll] for(uint k=0;k<4;++k) {
  p+=mul(float4(i.p,1),bodies[actor].joints[joints[k]]).xyz*weights[k];
  n+=mul(float4(i.n,0),bodies[actor].normals[joints[k]]).xyz*weights[k];
 }
 float4 style=bodies[actor].colors[(uint)i.tint.w];
 i.p=p; i.n=normalize(n); i.tint=float4(style.rgb,1); i.surface.x=style.w;
}
VOut VSBody(SkinIn v) {
 float3 bindPosition=v.object.p,bindNormal=v.object.n;
 SkinBody(v.object,v.bones,v.weights);
 VOut o=VSWorld(v.object); o.tp=bindPosition; o.tn=bindNormal; return o;
}
float4 VSBodyShadow(SkinIn v) : SV_POSITION {
 SkinBody(v.object,v.bones,v.weights); return VSShadow(v.object);
}
// Finger palettes contain rigid transforms only, shared by color and shadows.
cbuffer Hands : register(b2) { matrix handJoints[1024]; };
void SkinHand(inout InstanceIn i,uint4 joints,float4 weights) {
 uint base=(uint)i.surface.z*16;
 float3 p=0,n=0;
 [unroll] for(uint k=0;k<4;++k) {
  p+=mul(float4(i.p,1),handJoints[base+joints[k]]).xyz*weights[k];
  n+=mul(float4(i.n,0),handJoints[base+joints[k]]).xyz*weights[k];
 }
 i.p=p; i.n=normalize(n);
}
VOut VSHand(SkinIn v) {
 float3 bindPosition=v.object.p,bindNormal=v.object.n;
 SkinHand(v.object,v.bones,v.weights);
 VOut o=VSWorld(v.object); o.tp=bindPosition; o.tn=bindNormal; return o;
}
float4 VSHandShadow(SkinIn v) : SV_POSITION {
 SkinHand(v.object,v.bones,v.weights); return VSShadow(v.object);
}
)" R"(
float Hash(float2 p) { float3 q=frac(float3(p.xyx)*.1031); q+=dot(q,q.yzx+33.33); return frac((q.x+q.y)*q.z); }
float Noise(float2 p) {
 float2 cell=floor(p),f=frac(p); f=f*f*(3-2*f);
 return lerp(lerp(Hash(cell),Hash(cell+float2(1,0)),f.x),lerp(Hash(cell+float2(0,1)),Hash(cell+1),f.x),f.y);
}
// Ground-level daylight approximation: wavelength-dependent extinction, Rayleigh
// phase and forward Mie scattering. Curvature bounds the horizon optical path.
float3 SunDirection() { return normalize(sun.xyz); }
float3 AirMass(float elevation) {
 float mu=max(elevation,0);
 return float3(8000*rsqrt(mu*mu+.0008),1200*rsqrt(mu*mu+.00012),0);
}
float3 SunTransmission() {
 float3 path=AirMass(SunDirection().y);
 return exp(-float3(5.8,13.5,33.1)*.000001*path.x-.000004*path.y);
}
float3 Daylight() { return float3(2.5,2.44,2.30)*SunTransmission(); }
float3 SkyRadiance(float3 ray) {
 float3 light=SunDirection();
 float cosine=clamp(dot(ray,light),-1,1);
 float3 path=AirMass(ray.y);
 float3 rayDepth=float3(5.8,13.5,33.1)*.000001*path.x;
 float mieDepth=.000004*path.y;
 float3 extinction=rayDepth+mieDepth;
 float rayPhase=.0596831*(1+cosine*cosine);
 const float g=.76;
 float miePhase=.0795775*(1-g*g)/pow(max(1+g*g-2*g*cosine,.001),1.5);
 const float forwardG=.94;
 float forwardPhase=.0795775*(1-forwardG*forwardG)/pow(max(1+forwardG*forwardG-2*forwardG*cosine,.0001),1.5);
 miePhase=lerp(miePhase,forwardPhase,.15);
 float3 scatter=(1-exp(-extinction))*(rayDepth*rayPhase+mieDepth*miePhase)/max(extinction,.00001);
 float3 radiance=scatter*SunTransmission()*15.5;
 radiance+=float3(.018,.035,.065)*(1-exp(-path.x/18000));
 // Keep the floating island's lower hemisphere cool and deep rather than a white void.
 radiance*=lerp(.72,1,smoothstep(0,.22,ray.y));
 return lerp(float3(.16,.23,.34),radiance,smoothstep(-.45,.02,ray.y));
}
float3 DisplayDaylight(float3 radiance) {
 float3 x=max(radiance,0)*atmosphere.y;
 // Fitted filmic shoulder; sky, sun, surfaces and aerial perspective share it.
 float3 mapped=saturate((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14));
 return pow(mapped,1/2.2);
}
float2 CloudDrift() { return float2(cos(atmosphere.x)-1,sin(atmosphere.x))*.65; }
float CloudOctave(float2 p,float frequency,float offset,float footprint) {
 float weight=1-smoothstep(.35,.8,footprint*frequency);
 return lerp(.5,Noise(p*frequency+offset),weight);
}
float3 CloudPoint(float3 ray,float height) {
 // A bounded curved deck: height is distance through its base/top, not screen UV.
 float2 p=ray.xz/(max(ray.y,0)+.20)*2.7*(1+height*.32);
 return float3(p.x,height,p.y)+float3(4.9+CloudDrift().x,0,1.2+CloudDrift().y);
}
float CloudBillow(float3 p,float footprint) {
 float layer=floor(p.y),fraction=frac(p.y); fraction=fraction*fraction*(3-2*fraction);
 return lerp(CloudOctave(p.xz,1,layer*19.17,footprint),
   CloudOctave(p.xz,1,(layer+1)*19.17,footprint),fraction);
}
float CloudVolume(float3 p,float footprint,bool detail) {
 float h=p.y;
 float profile=smoothstep(0,.20,h)*(1-smoothstep(.60,1,h));
 float3 q=float3(p.x,h*1.6,p.z);
 float shape=CloudBillow(q,footprint)*.38+
   CloudBillow(q*2.03+7.1,footprint*2.03)*.29+
   CloudBillow(q*4.11-3.7,footprint*4.11)*.21+
   CloudBillow(q*8.23+13.6,footprint*8.23)*.12;
 // Broader at the base, with narrowing rounded tops and a soft condensation floor.
 float threshold=atmosphere.w+.12*smoothstep(.35,1,h)-.030;
 float erosion=.5;
 if (detail) {
  erosion=CloudBillow(q*8.23+13.6,footprint*8.23)*.65+
    CloudBillow(q*16.47-19.1,footprint*16.47)*.35;
 }
 float density=max(0,(shape-threshold)*12-(1-erosion)*.65);
 return density*profile*3;
}
float CloudSunlight(float3 position) {
 // Broad cloud attenuation is evaluated per world vertex. Twelve samples avoid
 // missing thin cloud lobes and leave fine erosion to the sky pass.
 float3 light=SunDirection();
 float opticalDepth=0;
 [loop] for (int j=0;j<12;++j) {
  float3 p=CloudPoint(light,(j+.5)/12.0); p.xz+=position.xz*.0005;
  opticalDepth+=CloudVolume(p,0,false);
 }
 return exp(-opticalDepth*(.50/(light.y+.20))*13/12);
}
float SurfaceCloudSunlight(float3 position) {
 float value=1;
 [branch] if (cloudField.x>0 && all(abs(position.xz)<=cloudField.y))
  value=cloudLightField.SampleLevel(fontSampler,position.xz/(2*cloudField.x)+.5,0);
 // Preserve lighting for geometry outside the bounded field, too.
 else value=CloudSunlight(position);
 return value;
}
float4 CloudRadiance(float3 ray,float footprint,float sampleOffset) {
 if (ray.y<=.015) return float4(0,0,0,1);
 float3 light=SunDirection(),sky=SkyRadiance(float3(0,1,0)),day=Daylight();
 float transmission=1; float3 scattered=0;
 float horizon=smoothstep(.015,.11,ray.y);
 float stepLength=.50/(max(ray.y,0)+.20)/48;
 // Also filter noise along a march segment, not only across a screen pixel.
 footprint=max(footprint,length(float3(ray.xz*.864/(ray.y+.20),1.6))/48*.55);
 float forward=pow(saturate(dot(ray,light)),10);
 // Integrate a shallow volume front to back; explicit footprint is computed before
 // branches, so density and light taps never evaluate derivatives in divergent flow.
 [loop] for (int j=0;j<48;++j) {
  float h=(j+sampleOffset)/48.0;
  float3 p=CloudPoint(ray,h);
  float density=CloudVolume(p,footprint,true)*horizon;
  [branch] if (density>.001) {
   float shadow=CloudVolume(p+light*.20,footprint,false)*.20+
     CloudVolume(p+light*.50,footprint,false)*.30;
   float direct=exp(-shadow*13);
   float multiple=.24*exp(-shadow*1.5);
   float3 illumination=sky*lerp(.42,.85,h)+day*(.08+direct*(.62+forward*.85)+multiple);
   float opacity=1-exp(-density*stepLength*13);
   scattered+=transmission*opacity*illumination;
   transmission*=1-opacity;
  }
  if (transmission<.0001) { transmission=0; break; }
 }
 // Distant cloud banks merge into the horizon air instead of ending in a cutout.
 float haze=1-exp(-.055/(max(ray.y,0)+.08));
 scattered=lerp(scattered,SkyRadiance(ray)*(1-transmission),haze);
 return float4(scattered,transmission);
}
float AirTransmission(float3 from,float3 to) {
 float distance=max(0,length(to-from)-8);
 float density=atmosphere.z*exp(-max(0,(from.y+to.y)*.5)*.035);
 return exp(-distance*density);
}

float3 ShadowCoordinate(float4 position) {
 float3 projected=position.xyz/position.w;
 float2 uv=projected.xy*float2(.5,-.5)+.5;
 return float3(uv,projected.z);
}
)"
R"(
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
float Shadow(float4 widePosition,float4 detailPosition,float sunFacing,float receiverPlane) {
 float3 wide=ShadowCoordinate(widePosition),detail=ShadowCoordinate(detailPosition);
 // Compute both receiver planes before any cascade-dependent branch; derivatives
 // inside a divergent selection can introduce stripes at the blend boundary.
 float2 wideGradient=ShadowGradient(wide)*receiverPlane,detailGradient=ShadowGradient(detail)*receiverPlane;
 float margin=min(min(detail.x,1-detail.x),min(detail.y,1-detail.y));
 // Fade over 5.4 m of the 20 m detail map, retaining full detail in its central
 // 8 m. The old 2.4 m band made thin shadows visibly pop under bright daylight.
 float weight=smoothstep(.03,.30,margin);
 if (detail.z<0 || detail.z>1) weight=0;
 float nearValue=1,farValue=1;
 [branch] if (weight>0) nearValue=ShadowFilter(detail,detailGradient,sunFacing,1);
 [branch] if (weight<1) farValue=ShadowFilter(wide,wideGradient,sunFacing,0);
 return lerp(farValue,nearValue,weight);
}
float4 PSMain(VOut i) : SV_TARGET {
 if (eyeAndHud.w>.5) {
  float4 result=color*i.tint;
  if (i.wp.z>4.5) result*=uiIcons.Sample(fontSampler,i.wp.xy);
  else if (i.wp.z>3.5) result*=uiHall.Sample(fontSampler,i.wp.xy);
  else if (i.wp.z>2.5) result*=uiFrame.Sample(fontSampler,i.wp.xy);
  else if (i.wp.z>1.5) result*=mapArtwork.Sample(fontSampler,i.wp.xy);
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
 float3 lightDir=normalize(eyeAndHud.w<0?float3(-.3,.6,1):sun.xyz);
 float diffuse=saturate(dot(n,lightDir));
 float visibility=eyeAndHud.w<0?1:Shadow(i.shadow,i.detailShadow,saturate(dot(geometric,lightDir)),i.receiverPlane);
 if (eyeAndHud.w>=0) visibility*=saturate(i.cloudLight);
 float hemisphere=lerp(.20,.58,n.y*.5+.5);
 // A small transmitted-light term keeps the backs of opaque leaves readable.
 if (i.material==8) { hemisphere=max(hemisphere,.48); diffuse+=saturate(dot(-n,lightDir))*.32; }
 float3 base=pow(saturate(albedo),2.2);
 float metal=i.material==7?.8:0;
 float3 view=normalize(eyeAndHud.xyz-i.wp),halfVector=normalize(lightDir+view);
 float exponent=lerp(96,5,roughness*roughness);
 float3 fresnel=lerp(.035,base,metal);
 fresnel+=(1-fresnel)*pow(1-saturate(dot(view,halfVector)),5);
 float3 skyFill=eyeAndHud.w<0?float3(.52,.68,.91):SkyRadiance(float3(0,1,0))*1.4+float3(.12,.13,.14);
 float3 ambient=lerp(float3(.29,.23,.16),skyFill,n.y*.5+.5)*hemisphere;
 float3 sunlight=eyeAndHud.w<0?float3(1,.88,.70)*2.15:Daylight();
 float specular=pow(saturate(dot(n,halfVector)),exponent)*(exponent+8)/32;
 float3 result=base*(ambient*cavity+sunlight*diffuse*visibility)*(1-metal*.45);
 result+=fresnel*(specular*sunlight*visibility*diffuse+ambient*.45);
 if (i.material==10) result=base*2.3;
 if (eyeAndHud.w<0) return float4(pow(max(0,result/(1+result)),1/2.2),1);
 // Extinction and in-scattered sky light mix before tone mapping, not in display RGB.
 float transmission=AirTransmission(eyeAndHud.xyz,i.wp);
 float3 sight=normalize(i.wp-eyeAndHud.xyz);
 float3 haze=SkyRadiance(normalize(float3(sight.x,max(sight.y,.035),sight.z)));
 result=lerp(haze,result,transmission);
 return float4(DisplayDaylight(result),1);
}
struct EffectOut { float4 p : SV_POSITION; float3 uv : TEXCOORD0; float4 tint : COLOR; float fog : TEXCOORD1; };
EffectOut VSEffect(VIn i) {
 EffectOut o; o.p=mul(float4(i.p,1),wvp); o.uv=i.n; o.tint=i.tint;
 o.fog=AirTransmission(eyeAndHud.xyz,i.p); return o;
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
 SkyOut o; o.uv=float2((id<<1)&2,id&2); o.p=float4(o.uv*float2(2,-2)+float2(-1,1),1,1); return o;
}
float PSCloudLight(SkyOut i) : SV_TARGET {
 float2 position=(i.uv*2-1)*cloudField.x;
 return CloudSunlight(float3(position.x,0,position.y));
}
[earlydepthstencil]
float4 PSSky(SkyOut i) : SV_TARGET {
 // The sky pass uses the existing world matrix slot for inverse view-projection.
 // Direction changes with orbit/pitch/aspect, but ignores camera translation.
 float4 farPosition=mul(float4(i.uv*float2(2,-2)+float2(-1,1),1,1),world);
 float3 ray=normalize(farPosition.xyz/farPosition.w-eyeAndHud.xyz);
 float3 light=SunDirection();
 float3 radiance=SkyRadiance(ray);
 float angle=length(ray-light),width=max(fwidth(angle),.0001);
 float disc=1-smoothstep(max(0,sun.w-width),sun.w+width,angle);
 // A small optical aureole surrounds the real angular disc. Both are attenuated
 // by cloud transmittance and rendered behind scene depth, so neither leaks through roofs.
 float glow=1.8*exp(-angle*angle/.00065)+.24*exp(-angle*angle/.0064);
 float3 solar=SunTransmission()*(disc*180+glow);
 float3 deck=CloudPoint(ray,0);
 float footprint=max(length(ddx(deck.xz)),length(ddy(deck.xz)))*1.32;
 // Static sub-step dither breaks slice contours without frame-varying noise.
 float sampleOffset=.15+.70*Hash(floor(i.p.xy));
 float4 clouds=CloudRadiance(ray,footprint,sampleOffset);
 radiance=clouds.rgb+(radiance+solar)*clouds.a;
 // Sparse high-altitude ice streaks, softened by the same pixel footprint.
 float2 high=ray.xz/(max(ray.y,0)+.35)*float2(3.5,13)+CloudDrift()*.4;
 float highFootprint=max(length(ddx(high)),length(ddy(high)));
 float veil=smoothstep(.64,.84,CloudOctave(high,1,0,highFootprint)*.7+
   CloudOctave(high,2.1,7.3,highFootprint)*.3)*smoothstep(.02,.18,ray.y)*.055;
 radiance=lerp(radiance,SkyRadiance(float3(0,1,0))*.6+Daylight()*.45,veil*clouds.a);
 return float4(DisplayDaylight(radiance),1);
}
)";
