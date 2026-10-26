#include "engine/Geometry.h"
#include "engine/InstanceTransform.h"
#include "game/WorldMap.h"
#include "game/CharacterPose.h"
#include "game/TerrainSurface.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool condition,const char* message) {
  if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
bool Finite(DirectX::XMFLOAT3 p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
float Cross2(DirectX::XMFLOAT3 a,DirectX::XMFLOAT3 b,DirectX::XMFLOAT3 p) {
  return (b.x-a.x)*(p.z-a.z)-(b.z-a.z)*(p.x-a.x);
}
bool Covers(const visual::Geometry& g,DirectX::XMFLOAT3 p) {
  for (std::size_t i=0;i<g.indices.size();i+=3) {
    const auto a=g.vertices[g.indices[i]].position,b=g.vertices[g.indices[i+1]].position,c=g.vertices[g.indices[i+2]].position;
    const float x=Cross2(a,b,p),y=Cross2(b,c,p),z=Cross2(c,a,p);
    if ((x>=-.001f && y>=-.001f && z>=-.001f) || (x<=.001f && y<=.001f && z<=.001f)) return true;
  }
  return false;
}
}
int main() {
  using namespace DirectX;
  // Compare the optimized instance path to the general inverse, including
  // nonuniform scale, arbitrary rotation and distant translated objects.
  for (int i=0;i<120;++i) {
    const float x=.01f+i*.17f,y=.03f+(i%7)*.31f,z=.02f+(i%11)*.73f;
    const auto world=XMMatrixScaling(x,y,z)*XMMatrixRotationRollPitchYaw(i*.13f,i*.37f,-i*.21f)*
      XMMatrixTranslation(i*13.f,-i*3.f,i*7.f);
    const auto prepared=visual::PrepareInstanceTransform(world);
    const auto reference=XMMatrixTranspose(XMMatrixInverse(nullptr,world));
    for (auto normal : {XMVectorSet(1,0,0,0),XMVectorSet(0,1,0,0),XMVectorSet(0,0,1,0),XMVectorSet(.3f,.7f,-.4f,0)}) {
      const auto a=XMVector3Normalize(XMVector3TransformNormal(normal,prepared.normal));
      const auto b=XMVector3Normalize(XMVector3TransformNormal(normal,reference));
      Check(XMVectorGetX(XMVector3Length(a-b))<.000001f,"Optimized normal transform changed nonuniform-scale lighting.");
    }
    Check(std::abs(prepared.radiusScale-std::max({x,y,z}))<.00001f,"Optimized transform changed culling bounds.");
  }
  const auto coat=visual::BuildMesh(visual::Mesh::CoatPanel);
  Check(coat.vertices.size()==198 && coat.indices.size()==960,"Coat panel must retain bounded two-sided topology");
  for(const auto& v:coat.vertices)
    Check(std::abs(v.position.x)<=.501f && v.position.y>=-.501f && v.position.y<=.501f &&
      v.position.z>=.65f && v.position.z<=1.01f,"Tailored coat escaped its attachment envelope");
  for(auto mesh:{visual::Mesh::Helm,visual::Mesh::HelmLow}) {
    const auto helm=visual::BuildMesh(mesh);
    for(const auto& v:helm.vertices)
      Check(std::abs(v.position.x)<=.501f && std::abs(v.position.z)<=.501f &&
        v.position.y>=-.501f && v.position.y<=.531f,"Helmet must retain its fitted head envelope");
  }
  std::size_t bytes=0,triangles=0,weaponBytes=0,weaponTriangles=0,armorBytes=0,armorTriangles=0;
  for (unsigned mesh=0;mesh<static_cast<unsigned>(visual::Mesh::Count);++mesh) {
    const auto g=visual::BuildMesh(static_cast<visual::Mesh>(mesh));
    Check(!g.vertices.empty() && !g.indices.empty() && g.indices.size()%3==0,"Missing or incomplete mesh.");
    // The separately budgeted weighted body is covered by SkinningTests.
    if(!visual::IsBodyMesh(static_cast<visual::Mesh>(mesh)) && !visual::IsHandMesh(static_cast<visual::Mesh>(mesh))) {
      const auto cost=g.vertices.size()*sizeof(visual::Vertex)+g.indices.size()*sizeof(std::uint32_t);
      if(mesh>=unsigned(visual::Mesh::GuardCuirass) && mesh<=unsigned(visual::Mesh::GuardCloak)) {armorBytes+=cost;armorTriangles+=g.indices.size()/3;}
      else if(mesh>=unsigned(visual::Mesh::WeaponMace)) {weaponBytes+=cost;weaponTriangles+=g.indices.size()/3;}
      else {bytes+=cost;triangles+=g.indices.size()/3;}
    }
    for (const auto& v : g.vertices) {
      Check(Finite(v.position) && Finite(v.normal),"Non-finite mesh data would poison bounds or lighting.");
      const float length=XMVectorGetX(XMVector3Length(XMLoadFloat3(&v.normal)));
      Check(std::abs(length-1)<.001f,"Lighting requires normalized vertex normals.");
    }
    for (auto index : g.indices) Check(index<g.vertices.size(),"GPU index exceeds the vertex buffer.");
    for (std::size_t i=0;i<g.indices.size();i+=3) {
      const auto& a=g.vertices[g.indices[i]]; const auto& b=g.vertices[g.indices[i+1]]; const auto& c=g.vertices[g.indices[i+2]];
      const auto normal=XMVector3Cross(XMLoadFloat3(&b.position)-XMLoadFloat3(&a.position),XMLoadFloat3(&c.position)-XMLoadFloat3(&a.position));
      // Hands are authored directly in metres (unlike normalized prop meshes).
      const float minimumArea=visual::IsHandMesh(static_cast<visual::Mesh>(mesh))?1e-18f:1e-12f;
      Check(XMVectorGetX(XMVector3LengthSq(normal))>minimumArea,"Degenerate procedural triangle.");
      const auto average=XMLoadFloat3(&a.normal)+XMLoadFloat3(&b.normal)+XMLoadFloat3(&c.normal);
      if (XMVectorGetX(XMVector3Dot(normal,average))<=0) {
        std::cerr << "Mesh " << mesh << ", triangle " << i/3 << ": ";
        Check(false,"Vertex normals oppose the triangle winding.");
      }
    }
  }
  Check(bytes<=640*1024 && triangles<=16000,"Reusable mesh library exceeded its startup geometry budget.");
  std::cout<<"Guard armor: "<<armorBytes<<" bytes / "<<armorTriangles<<" triangles\n";
  Check(armorBytes<=320*1024 && armorTriangles<=8500,"Guard outfit exceeded its bounded shared geometry budget");
  for(auto mesh:{visual::Mesh::GuardCuirass,visual::Mesh::GuardPlate}) {
    const auto high=visual::BuildMesh(mesh),low=visual::BuildMesh(visual::AttachmentMesh(mesh,1));
    Check(low.indices.size()<high.indices.size()/2,"Guard armor must reduce topology at distance");
    for(const auto& v:high.vertices) Check(std::abs(v.position.x)<=.51f && std::abs(v.position.y)<=.51f && std::abs(v.position.z)<=.51f,
      "Guard armor escaped its authored attachment envelope");
  }
  std::cout<<"Equipment budget measured: "<<weaponBytes<<" bytes / "<<weaponTriangles<<" triangles\n";
  Check(weaponBytes<=416*1024 && weaponTriangles<=8500,"Class equipment exceeded its separate cached geometry budget.");
  std::cout<<"Class weapon bytes: "<<weaponBytes<<"; triangles: "<<weaponTriangles<<'\n';
  const auto cloak=visual::BuildMesh(visual::Mesh::Cloak);
  for (const auto& vertex : cloak.vertices) {
    const auto p=vertex.position;
    Check(std::abs(p.x)<=.361f && p.y>=-.821f && p.y<=0 && p.z<0,
      "Resting cloth must stay behind the shoulder seam and above the knees.");
  }
  const rpg::Obstacle stall{{3,-13.3f},1.1f,.4f,.85f,rpg::PropKind::Counter};
  for (const auto& vertex : visual::BuildMesh(visual::Mesh::Canopy).vertices) {
    const auto p=vertex.position;
    Check(std::abs(p.x*2.75f)<stall.CameraHalfX() && std::abs(p.z*1.35f)<stall.CameraHalfZ() &&
      p.y+2.63f<stall.CameraHeight() && p.y+2.63f>2.05f,
      "Canopy geometry must fit the upper camera envelope and clear the open bay.");
  }
  for (const auto& vertex : visual::BuildMesh(visual::Mesh::Arch).vertices) {
    const auto p=vertex.position;
    Check(std::abs(p.x)<=.5f && p.y>=0 && p.y<=.5f && std::abs(p.z)<=.5f,
      "Reusable masonry arch must stay inside its authored landmark bounds.");
    Check(std::hypot(p.x,p.y)>=.359f,"Arch masonry must not fill its intended opening.");
  }
  for (const auto& vertex : visual::BuildMesh(visual::Mesh::Foliage).vertices) {
    const auto p=vertex.position;
    Check(std::hypot(p.x,p.z)<.6f && std::abs(p.y)<.5f,
      "Leaf crowns exceed the tree camera envelope at their authored scale.");
    Check(.30f+p.y*.65f>=0,"Shrub leaves penetrate the gameplay floor.");
  }
  for (const auto& vertex : visual::BuildMesh(visual::Mesh::Grass).vertices)
    Check(vertex.position.y>=0 && vertex.position.y<.5f && std::hypot(vertex.position.x,vertex.position.z)<.35f,
      "Grass must fit its placement clearance and remain above ground.");
  // Check pose outcomes across the whole gait/attack envelope, including malformed input.
  const auto length=[](XMFLOAT3 a,XMFLOAT3 b) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a)-XMLoadFloat3(&b))); };
  for (bool shield : {false,true}) for (int step=0;step<=40;++step) for (int attack=0;attack<=20;++attack) {
    const auto pose=BuildCharacterPose(attack/20.f,step/20.f-1,shield);
    for (int side=0;side<2;++side) {
      const auto& arm=pose.arms[side]; const auto& leg=pose.legs[side];
      Check(Finite(arm.root) && Finite(arm.joint) && Finite(arm.end) && Finite(leg.joint) && Finite(leg.end),"Non-finite animated joints.");
      Check(std::abs(length(arm.root,arm.joint)-CharacterPose::upperArm)<.0001f &&
        std::abs(length(arm.joint,arm.end)-CharacterPose::forearm)<.0001f,"Attack pose stretches the arm.");
      Check(std::abs(length(leg.root,leg.joint)-CharacterPose::thigh)<.0001f &&
        std::abs(length(leg.joint,leg.end)-CharacterPose::shin)<.0001f,"Walking pose stretches the leg.");
      Check(leg.end.y>=.17f-.0001f && leg.joint.z>-.12f,"Foot penetrates ground or knee bends backward.");
    }
    Check(std::abs(length(pose.weaponDirection,{0,0,0})-1)<.0001f,"Weapon attachment direction must be normalized.");
    const float tipY=pose.arms[1].end.y+pose.weaponDirection.y*.96f;
    Check(tipY>.1f,"Resting blade penetrates the floor.");
  }
  const auto neutral=BuildCharacterPose(0,0,false);
  Check(std::abs(neutral.legs[0].joint.x+neutral.legs[1].joint.x)<.0001f &&
    length(neutral.legs[0].end,{-.18f,.17f,0})<.0001f,"Neutral pose must retain a symmetric stance.");
  for (float invalid : {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-100.f,100.f}) {
    const auto pose=BuildCharacterPose(invalid,invalid,true);
    Check(Finite(pose.arms[1].end) && Finite(pose.legs[0].joint) && Finite(pose.weaponDirection),"Pose inputs must sanitize safely.");
  }
  const auto terrain=visual::BuildMesh(visual::Mesh::Terrain);
  for (int x=-int(rpg::WorldMap::halfWidth);x<=int(rpg::WorldMap::halfWidth);++x)
    for (int z=-int(rpg::WorldMap::halfDepth);z<=int(rpg::WorldMap::halfDepth);++z)
    Check(Covers(terrain,{float(x),0,float(z)}),"Visible terrain leaves a hole inside the playable collision rectangle.");
  for (const auto& v : terrain.vertices) Check(v.position.y==0 && v.normal.y>.999f,"Terrain must match the flat gameplay floor.");
  const auto cliff=visual::BuildMesh(visual::Mesh::Cliff);
  for (std::size_t i=1;i<terrain.vertices.size();++i) {
    const auto a=terrain.vertices[i].position,b=cliff.vertices[i-1].position;
    Check(a.x==b.x && a.y==b.y && a.z==b.z,"Island edge and cliff must join without a crack.");
  }
  XMFLOAT4X4 camera;
  XMStoreFloat4x4(&camera,XMMatrixPerspectiveFovLH(XM_PIDIV2,1,1,20));
  Check(visual::VisibleSphere({0,0,5},.5f,camera),"Visible object was culled.");
  Check(visual::VisibleSphere({5.3f,0,5},.5f,camera),"Object intersecting the frustum edge was culled.");
  Check(!visual::VisibleSphere({0,0,-5},.5f,camera),"Object behind the camera was retained.");
  Check(!visual::VisibleSphere({0,0,30},.5f,camera),"Object beyond the far plane was retained.");
  Check(!visual::VisibleSphere({20,0,5},.5f,camera),"Object outside the side plane was retained.");
  std::size_t textureBytes=visual::shadowSize*visual::shadowSize*4*visual::shadowLayers;
  Check(visual::BuildMaterialLayer(visual::Material::Leather,128)!=
    visual::BuildMaterialLayer(visual::Material::Cloth,128),"Leather must not reuse cloth weave");
  Check(visual::BuildMaterialLayer(visual::Material::Hair,128)!=
    visual::BuildMaterialLayer(visual::Material::Cloth,128),"Hair must not reuse cloth weave");
  for (unsigned layer=0;layer<static_cast<unsigned>(visual::Material::Count);++layer) {
    const auto pixels=visual::BuildMaterialLayer(static_cast<visual::Material>(layer),visual::materialSize);
    Check(pixels.size()==visual::materialSize*visual::materialSize*4,"Material upload dimensions are invalid.");
    Check(pixels==visual::BuildMaterialLayer(static_cast<visual::Material>(layer),visual::materialSize),
      "Procedural materials must be deterministic.");
    int relief=0;
    for (std::size_t i=0;i<pixels.size();i+=4) {
      Check(pixels[i+3]>=64,"Packed roughness must keep subpixel highlights broad enough to filter.");
      relief+=std::abs(int(pixels[i+1])-128)+std::abs(int(pixels[i+2])-128);
      if (layer==unsigned(visual::Material::Plain) || layer==unsigned(visual::Material::Glow))
        Check(pixels[i]==255 && pixels[i+1]==128 && pixels[i+2]==128,"Plain/glow layers must retain a neutral surface.");
    }
    if (layer==unsigned(visual::Material::Stone) || layer==unsigned(visual::Material::Wood) || layer==unsigned(visual::Material::Roof))
      Check(relief>1000,"Authored hard surfaces must contain a usable relief signal.");
    for (unsigned size=visual::materialSize;size;size/=2) textureBytes+=size*size*4;
  }
  for (unsigned size=terrainSurfaceSize;size;size/=2) textureBytes+=size*size*4;
  Check(textureBytes<10*1024*1024,"Materials, ground weights and both shadow maps exceeded the 10 MiB budget.");
  std::cout << "PASS: mesh topology, fixed-length poses, floor coverage, joined boundary and resource budgets. Mesh bytes: "<<bytes<<"; triangles: "<<triangles<<"\n";
}
