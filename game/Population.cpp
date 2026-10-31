#include "game/Simulation.h"
#include "game/Hunts.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace rpg {
namespace {int Bits(std::uint64_t n){int result=0;while(n){n&=n-1;++result;}return result;}
float Distance(Vec2 a,Vec2 b){return std::hypot(a.x-b.x,a.z-b.z);}}
void Simulation::PopulateBeasts() {
  beasts_.clear();beasts_.reserve(population::beastCount);
  for(int d=0;d<population::districtCount;++d) for(int i=0;i<population::beastsPerDistrict;++i) {
    const auto& district=population::districts[d];Enemy e;
    const float angle=i*1.04719755f+.35f,radius=i==5?2.f:6.f;
    const Vec2 desired{district.habitat.x+std::sin(angle)*radius,district.habitat.z+std::cos(angle)*radius};
    e.position=desired;
    for(int attempt=0;!map_.CanStand(e.position,.55f) && attempt<128;++attempt) {
      const float a=attempt*2.39996f,r=.5f+attempt*.08f;
      e.position={desired.x+std::sin(a)*r,desired.z+std::cos(a)*r};
    }
    e.home=e.position;e.rank=district.rank;e.kind=district.wolves?EnemyKind::Wolf:EnemyKind::Boar;
    e.elite=i==5;e.id=population::beastBase+unsigned(beasts_.size());e.health=e.MaxHealth();
    e.facing=angle>3.14159f?angle-6.2831853f:angle;e.roamTime=i*.7f;
    beasts_.push_back(e);
  }
}
int Simulation::HuntProgress(int i) const {
  if(i<0||i>=6)return 0;
  return Bits(huntMarks_[i]);
}
bool Simulation::HuntReady(int i) const {
  return i>=0 && i<6 && HuntProgress(i)>=HuntRequired(i) &&
    ((i!=3 && i!=4) || (huntMarks_[i]&32));
}
int Simulation::WildernessGold() const {
  int result=2*Bits(beastLoot_);
  for(int i=0;i<6;++i)if(hunts_[i]==2)result+=huntQuests[i].gold;
  return result;
}
int Simulation::WildernessExperience() const {
  int result=0;
  for(int i=0;i<36;++i)if(beastKills_&(std::uint64_t(1)<<i))result+=i%6==5?25:10;
  for(int i=0;i<6;++i)if(hunts_[i]==2)result+=huntQuests[i].experience;
  return result;
}
void Simulation::BeastDefeated(Enemy& e,int oldLevel) {
  const auto index=e.id-population::beastBase;const int district=int(index)/6;
  const auto bit=std::uint64_t(1)<<index;const bool first=!(beastKills_&bit);
  beastKills_|=bit;e.health=0;e.respawn=120;e.looted=false;
  if(hunts_[district]==1 && !HuntGather(district)) huntMarks_[district]|=1u<<(index%6);
  if(first) {Emit(FeedbackKind::Experience,e.position,e.elite?25:10);RestoreForLevel(oldLevel);}
  message_=HuntReady(district) && hunts_[district]==1?"Commission ready. Return to its speaker.":"Beast defeated. Press E nearby to search its remains.";
}
void Simulation::UpdateBeasts(float dt) {
  if(ReplayActive())return;
  for(auto& e:beasts_) {
    if(e.health<=0) {
      e.respawn=std::max(0.f,e.respawn-dt);
      // Never materialize beside the player or restore through a load/reset trick.
      if(e.respawn==0 && Distance(player_,e.home)>22 && Distance(player_,e.position)>22) {
        e.position=e.home;e.health=e.MaxHealth();e.state=EnemyState::Idle;e.looted=false;
        e.windup=e.attackCooldown=e.movementSpeed=0;e.path.clear();
      }
      continue;
    }
    if(e.state!=EnemyState::Idle || Distance(player_,e.home)>42 || e.rooted>0 || e.stunned>0)continue;
    // Bounded, deterministic habitat roaming. Home is a leash anchor, not a
    // moving target; pathfinding is only needed once combat has begun.
    e.roamTime+=dt;
    const float phase=std::fmod(e.roamTime,15.f);
    if(phase<5) {e.movementSpeed=0;continue;}
    const float a=e.id*.7f+std::floor(e.roamTime/15.f)*2.4f;
    const Vec2 goal{e.home.x+std::sin(a)*2.5f,e.home.z+std::cos(a)*2.5f};
    const float distance=Distance(e.position,goal);
    if(distance<.1f || !map_.CanTravel(e.position,goal,.45f))continue;
    const float heading=std::atan2(goal.x-e.position.x,goal.z-e.position.z);
    e.facing=motion::Turn(e.facing,heading,4.f,dt);
    const float step=std::min(distance,.65f*dt*(e.snared>0?.45f:1.f));const auto before=e.position;
    MoveEnemy(e,{(goal.x-e.position.x)/distance*step,(goal.z-e.position.z)/distance*step});
    e.movementDistance+=Distance(before,e.position);
  }
}
Vec2 Simulation::HuntObjective(int i) const {
  if(i<0||i>=6)return player_;
  if(hunts_[i]!=1 || HuntReady(i))return WorldMap::residents[9+i].position;
  Vec2 result=population::districts[i].habitat;float nearest=10000;
  if(HuntGather(i)) {
    for(int j=0;j<3;++j)if(!(huntMarks_[i]&(1u<<j))) {
      const auto p=population::Cache(i,j);const float d=Distance(player_,p);
      if(d<nearest){nearest=d;result=p;}
    }
  } else for(int j=0;j<6;++j) {
    const auto& e=beasts_[i*6+j];if(e.health<=0 || (huntMarks_[i]&(1u<<j)))continue;
    if((i==3||i==4) && HuntProgress(i)>=2 && !(huntMarks_[i]&32) && j!=5)continue;
    const float d=Distance(player_,e.position);if(d<nearest){nearest=d;result=e.position;}
  }
  return result;
}
std::string Simulation::HuntSummary(int i) const {
  if(i<0||i>=6)return "";
  if(HuntReady(i))return std::string("Return to ")+huntQuests[i].speaker;
  return std::string(huntQuests[i].objective)+"  "+std::to_string(std::min(HuntProgress(i),HuntRequired(i)))+" / "+std::to_string(HuntRequired(i))+
    ((i==3||i==4) && !(huntMarks_[i]&32)?" / Leader still alive":"");
}
void Simulation::ConfirmHunt(QuestAction action) {
  const int i=HuntIndex(questDialogue_);
  if(i<0||i>=6)return;
  if(action==QuestAction::AcceptHunt && hunts_[i]==0) {
    hunts_[i]=1;huntMarks_[i]=0;trackedHunt_=i;message_=huntQuests[i].objective;
  } else if(action==QuestAction::CompleteHunt && hunts_[i]==1 && HuntReady(i)) {
    const int old=Level();hunts_[i]=2;gold_+=huntQuests[i].gold;RestoreForLevel(old);
    if(trackedHunt_==i)trackedHunt_=-1;
    Emit(FeedbackKind::Experience,player_,huntQuests[i].experience);message_=huntQuests[i].complete;
  }
}
std::string Simulation::SaveWilderness() const {
  std::ostringstream out;out<<std::setprecision(9)<<"WILDERNESS "<<beastKills_<<' '<<beastLoot_<<' '<<trackedHunt_;
  for(int i=0;i<6;++i)out<<' '<<hunts_[i]<<' '<<huntMarks_[i];
  out<<'\n';
  for(const auto& e:beasts_)out<<e.id<<' '<<e.position.x<<' '<<e.position.z<<' '<<std::max(0.f,e.health)<<' '
    <<int(e.state)<<' '<<e.looted<<' '<<e.attackCooldown<<' '<<e.windup<<' '<<e.facing<<' '<<e.respawn<<'\n';
  return out.str();
}
bool Simulation::LoadWilderness(const std::string& data) {
  std::istringstream in(data);std::string tag;
  constexpr std::uint64_t all=(std::uint64_t(1)<<36)-1;
  if(!(in>>tag>>beastKills_>>beastLoot_>>trackedHunt_) || tag!="WILDERNESS" ||
    beastKills_>all || (beastLoot_&~beastKills_) || trackedHunt_<-1 || trackedHunt_>=6)return false;
  for(int i=0;i<6;++i) {
    if(!(in>>hunts_[i]>>huntMarks_[i]) || hunts_[i]<0 || hunts_[i]>2 || huntMarks_[i]>(HuntGather(i)?7u:63u) ||
      (!hunts_[i] && huntMarks_[i]) || (hunts_[i]==2 && !HuntReady(i)))return false;
    if(!HuntGather(i) && (huntMarks_[i]&~unsigned((beastKills_>>(i*6))&63)))return false;
  }
  if(trackedHunt_>=0 && hunts_[trackedHunt_]!=1)return false;
  const auto valid=[](float n,float low,float high){return std::isfinite(n)&&n>=low&&n<=high;};
  for(auto& e:beasts_) {
    EntityId id=0;int state=0,looted=0;
    if(!(in>>id>>e.position.x>>e.position.z>>e.health>>state>>looted>>e.attackCooldown>>e.windup>>e.facing>>e.respawn) ||
      id!=e.id || !map_.CanStand(e.position,.45f) || Distance(e.position,e.home)>28 ||
      !valid(e.health,0,e.MaxHealth()) || state<0 || state>3 || looted<0 || looted>1 ||
      ((state==3)!=(e.health==0)) || (looted && e.health>0) ||
      !valid(e.attackCooldown,0,4) || !valid(e.windup,0,e.WindupDuration()) || !valid(e.facing,-3.142f,3.142f) ||
      !valid(e.respawn,0,120) || (e.health>0 && e.respawn>0) || (e.windup>0 && (state!=1||e.attackCooldown>0)))return false;
    const auto bit=std::uint64_t(1)<<(e.id-population::beastBase);
    if((e.health==0 && !(beastKills_&bit)) || (looted && !(beastLoot_&bit)))return false;
    e.state=EnemyState(state);e.looted=looted!=0;
  }
  in>>std::ws;return in.eof();
}
}
