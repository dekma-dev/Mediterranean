#include "game/CharacterView.h"
#include "engine/Renderer.h"
#include <cmath>
void DrawCharacter(Renderer& renderer,rpg::Vec2 p,float base,float yaw,DirectX::XMFLOAT4 color,float swing,float gait,bool armor,bool rewardWeapon) {
 using namespace DirectX; using M=visual::Mesh; using S=visual::Material;
    const XMFLOAT4 skin{.72f,.51f,.35f,1},leather{.20f,.14f,.095f,1},steel{.52f,.59f,.61f,1};
    const XMFLOAT4 blade=rewardWeapon?XMFLOAT4{.76f,.67f,.42f,1}:steel;
    const auto point=[&](float x,float h,float z) -> XMFLOAT3 {
      return {p.x+std::cos(yaw)*x+std::sin(yaw)*z,base+h,p.z-std::sin(yaw)*x+std::cos(yaw)*z};
    };
    const auto part=[&](M mesh,float x,float h,float z,XMFLOAT3 scale,XMFLOAT4 tint,S material) {
      renderer.DrawMesh(mesh,point(x,h,z),scale,tint,material,yaw);
    };
    part(M::Torso,0,1.18f,0,{.84f,.68f,.49f},armor?steel:color,armor?S::Metal:S::Cloth);
    part(M::Torso,0,.9f,0,{.76f,.18f,.52f},leather,S::Wood);
    part(M::Box,0,.93f,.23f,{.15f,.12f,.06f},{.67f,.49f,.22f,1},S::Metal);
    part(M::Limb,0,1.55f,0,{.2f,.2f,.2f},skin,S::Skin);
    part(M::Sphere,0,1.76f,.02f,{.36f,.46f,.36f},skin,S::Skin);
    part(M::Sphere,0,1.88f,-.025f,{.37f,.25f,.37f},{.19f,.13f,.08f,1},S::Cloth);
    part(M::Sphere,0,1.75f,.185f,{.075f,.11f,.1f},skin,S::Skin);
    // Small facial landmarks help the front view without texture assets.
    for (float side : {-1.f,1.f}) {
      part(M::Sphere,side*.09f,1.80f,.18f,{.035f,.027f,.025f},{.11f,.10f,.08f,1},S::Plain);
      const float stride=side*gait*.23f;
      const auto hip=point(side*.19f,.9f,0),knee=point(side*.21f,.52f,stride*.6f+.07f);
      const auto ankle=point(side*.21f,.18f,stride);
      renderer.DrawSegment(hip,knee,.32f,.33f,{.24f,.25f,.23f,1},S::Cloth);
      renderer.DrawSegment(knee,ankle,.27f,.29f,leather,S::Wood);
      part(M::Sphere,side*.21f,.12f,stride+.07f,{.27f,.24f,.43f},leather,S::Wood);
      const float lift=side>0?swing:0;
      const auto shoulder=point(side*.36f,1.42f,0);
      const auto elbow=point(side*.47f,1.16f+lift*.3f,-stride*.4f+.03f);
      const auto hand=point(side*.48f,1.0f+lift,-stride*.8f+.12f);
      renderer.DrawSegment(shoulder,elbow,.29f,.31f,color,S::Cloth);
      renderer.DrawSegment(elbow,hand,.22f,.24f,leather,S::Wood);
      renderer.DrawMesh(M::Sphere,hand,{.17f,.20f,.17f},skin,S::Skin,yaw);
      if (side>0) {
        renderer.DrawMesh(M::Blade,{hand.x,hand.y+.49f,hand.z},{.19f,.88f,.16f},blade,S::Metal,yaw);
        renderer.DrawMesh(M::Box,{hand.x,hand.y+.08f,hand.z},{.32f,.055f,.09f},blade,S::Metal,yaw);
      }
    }
    if (armor) {
      for (float side : {-1.f,1.f}) part(M::Sphere,side*.38f,1.44f,0,{.34f,.24f,.40f},steel,S::Metal);
    }
}

