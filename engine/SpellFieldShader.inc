R"(
struct SpellFieldData {float4 clock;float4 tint;float4 shape;float4 details;};
cbuffer SpellFields : register(b7) {SpellFieldData spellFields[12];};
float FieldLine(float distance,float width) {
 float aa=max(.0015,fwidth(distance));return 1-smoothstep(width,width+aa*1.5,abs(distance));
}
// Stable cells fracture along their shared edges; the reveal and cold veins
// move through them. No atlas image rotates beneath the player's feet.
float FieldFracture(float2 p) {
 float2 cell=floor(p),f=frac(p);float nearest=9,second=9;
 [unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++){
  float2 q=float2(x,y),h=float2(Hash(cell+q),Hash(cell+q+19.71));
  float d=length(q+.2+h*.6-f);
  if(d<nearest){second=nearest;nearest=d;}else second=min(second,d);
 }
 return FieldLine(second-nearest,.022);
}
float4 AttackFieldRadiance(EffectOut i,SpellFieldData f) {
 float2 p=i.uv.xy*f.clock.z;
 float radius=length(p),forward=dot(p,f.shape.xy),side=dot(p,float2(f.shape.y,-f.shape.x));
 float angle=abs(atan2(side,forward)),halfAngle=f.details.x;
 float cone=max(radius-f.clock.y,radius*sin(clamp(angle-halfAngle,-1.5707963,1.5707963)));
 if(halfAngle>3.1415)cone=radius-f.clock.y;
 float ring=f.shape.w>0?max(f.shape.z-radius,radius-f.shape.w):10000;
 float sd=min(cone,ring),aa=max(.008,fwidth(sd));
 float inside=1-smoothstep(-aa,aa,sd);
 float rim=1-smoothstep(.012,.012+aa*.65,abs(sd));
 float lip=1-smoothstep(.052,.052+aa*.65,abs(sd));
 // Stable engraved hatching inside the real danger shape. Only the softer
 // timing wave travels; the damage boundary is visible from the first frame.
 float hatch=FieldLine(sin((p.x+p.y)*8),.12)*inside;
 float phase=f.clock.x,front=lerp(.05,max(f.clock.y,f.shape.w),phase);
 float wave=exp2(-25*(radius-front)*(radius-front))*inside;
 float urgency=smoothstep(.55,1,phase),threat=f.details.y;
 float alpha=max(inside*(.10+hatch*.055+wave*.13+urgency*.035),lip*.26);
 alpha=max(alpha,rim*(.62+.20*threat));
 float3 ink=float3(.16,.055,.028),light=f.tint.rgb;
 float3 color=lerp(ink,light,saturate(rim+wave*.68+hatch*.24));
 color=lerp(color,float3(1,.87,.64)*light,rim*urgency*.32);
 float z=effectSceneDepth.Load(int3((int2)i.p.xy,0));
 float depth=28/max(.0001,280-z*279.9);
 return float4(color,alpha*i.fog*saturate((depth-i.viewDepth)/.02));
}
float4 FieldRadiance(EffectOut i) {
 SpellFieldData f=spellFields[min(11u,(uint)round(i.uv.z)-64u)];
 float4 result=0;
 [branch] if(f.clock.w>3.5){result=AttackFieldRadiance(i,f);}else{
 float2 p=i.uv.xy;float radius=length(p),angle=atan2(p.y,p.x),age=f.clock.x,seed=f.clock.y*.013;
 float edge=1-smoothstep(.91,1,radius),birth=smoothstep(0,.22,age);
 float reveal=1-smoothstep(saturate(age*2.8)-.12,saturate(age*2.8)+.03,radius);
 float strength=0;float3 color=0;
 float noise=Noise(p*5+seed),mist=Noise(p*4+float2(age*.12,-age*.16));
 if(f.clock.w<.5){
  float veins=FieldFracture(p*4.2+seed);
  float arms=pow(saturate(.5+.5*cos(angle*6+sin(radius*18)*.18)),12);
  float branches=FieldLine(sin(angle*18+radius*24),.045)*arms;
  float pulse=pow(saturate(.5+.5*sin(radius*21-age*3.8+noise*3)),6);
  strength=(veins*(.26+.42*pulse)+arms*.12+branches*.30+mist*.07)*reveal;
  color=lerp(float3(.10,.34,.62),float3(.54,.86,1),saturate(veins*.65+pulse*.28));
 }else if(f.clock.w<1.5){
  float cracks=FieldFracture(p*3.8+seed),cooling=exp(-age*.50);
  float hot=saturate(mist*1.4-.30),flow=Noise(p*9+float2(age*.25,seed));
  strength=(cracks*(.32+.58*flow)+hot*.13)*cooling*reveal;
  color=lerp(float3(.75,.035,.006),float3(1,.58,.075),hot);
 }else if(f.clock.w<2.5){
  // Original interlaced, eight-lobed tracery. Opposed rings carry a light
  // chase while the central/open negative spaces preserve terrain contrast.
  float petal=.51+.22*cos(angle*8);
  float lace=FieldLine(radius-petal,.010)+FieldLine(radius-(.52-.18*cos(angle*8)),.007)*.65;
  float rings=FieldLine(radius-.86,.008)+FieldLine(radius-.79,.004)*.65+FieldLine(radius-.25,.006);
  float ray=pow(saturate(.5+.5*cos(angle*16)),26)*smoothstep(.66,.71,radius)*(1-smoothstep(.75,.79,radius));
  float chase=pow(saturate(.5+.5*cos(angle*2-age*1.6)),6);
  // Suppress the angular derivative singularity at the empty centre.
  lace*=smoothstep(.14,.23,radius);
  strength=(lace*.55+rings*(.38+.32*chase)+ray*.72+mist*.035)*reveal;
  color=lerp(float3(.85,.40,.07),float3(1,.93,.64),saturate(lace*.6+chase*.5));
 }else{
  float spiral=FieldLine(sin(angle*3-radius*12+age*.8),.05);
  float rings=FieldLine(radius-.72,.008)+FieldLine(radius-.87,.005);
  strength=(spiral*.38*smoothstep(.10,.22,radius)+rings*.40)*reveal;
  color=float3(.49,.31,1);
 }
 float z=effectSceneDepth.Load(int3((int2)i.p.xy,0)),farPlane=eyeAndHud.w<0?20:280;
 float depth=(.1*farPlane)/max(.0001,farPlane-z*(farPlane-.1));
 float soft=saturate((depth-i.viewDepth)/.025);
 result=float4(color*f.tint.rgb,saturate(strength)*edge*birth*f.tint.a*i.fog*soft);
 }
 return result;
}
)"
