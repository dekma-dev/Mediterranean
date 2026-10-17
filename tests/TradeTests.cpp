#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>
#include <cmath>

void Require(bool value,const char* message) { if (!value) { std::cerr<<message<<'\n'; std::exit(1); } }
std::string Veteran() {
  return "MEDITERRANEAN 3\n3 -10 140 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
}
void Buy(rpg::Simulation& game,int index) { rpg::Command c; c.purchase=index; game.Update(.01f,c); }
int main() {
  using namespace rpg;
  Simulation game; Require(game.Load(Veteran()) && game.CanTrade(),"Veteran must reach the quartermaster.");
  const auto initial=game.Save(); Buy(game,-2); Buy(game,99);
  Require(game.Save()==initial,"Unknown purchases must not mutate gold, items or stock.");
  Buy(game,1); Buy(game,2);
  Require(game.Gold()==35 && game.Items().Count(Item::Helm)==1 && game.Items().Count(Item::Shield)==1,
    "Purchases must grant each item and charge the displayed price.");
  Require(game.ShopStock(1)==0 && game.ShopStock(2)==0 && game.DamageReduction()==2,
    "Bought gear must be stowed, not automatically equipped; unique stock must deplete.");
  const auto bought=game.Save(); Buy(game,1);
  Require(game.Save()==bought,"Sold-out equipment must not duplicate or consume more gold.");
  Command c; c.inventory={InventoryAction::Equip,game.Items().Find(Item::Helm),int(GearSlot::Head),Item::Helm}; game.Update(.01f,c);
  c.inventory={InventoryAction::Equip,game.Items().Find(Item::Shield),int(GearSlot::Offhand),Item::Shield}; game.Update(.01f,c);
  Require(game.DamageReduction()==5 && game.Experience()==300,"Six-slot protection must combine without changing earned XP.");
  Simulation copy; Require(copy.Load(game.Save()) && copy.Save()==game.Save() && copy.ShopStock(1)==0,
    "Version-5 saves must retain purchases, exact gold, stock and equipment.");
  // Armored training uses the real hit calculation, not only the attribute panel.
  for (auto waypoint : game.Map().FindPath(game.Player(),{12,-3.8f},.4f)) {
    for (int i=0;i<500 && std::hypot(game.Player().x-waypoint.x,game.Player().z-waypoint.z)>.15f;++i) {
      c={}; c.sprint=true; c.movement={waypoint.x-game.Player().x,waypoint.z-game.Player().z}; game.Update(.05f,c);
    }
  }
  c={}; c.interact=true; game.Update(.01f,c); Require(game.Sparring(),"Equipped veteran must start training.");
  for (int i=0;i<25 && game.SparringDamageTaken()==0;++i) game.Update(.05f,{});
  Require(game.SparringDamageTaken()==3,"Ward, helm and shield must reduce an actual 8-point hit to 3.");
  c={}; c.interact=true; game.Update(.01f,c); const auto remote=game.Save(); Buy(game,0);
  Require(game.Save()==remote,"Purchases away from the quartermaster must fail without side effects.");
  Require(game.Load(Veteran()),"Reset supply scenario.");
  for (int i=0;i<6;++i) Buy(game,0);
  Require(game.Potions()==11 && game.Gold()==54 && game.ShopStock(0)==0,"Supplies must obey bounded stock and charge every purchase.");
  Require(copy.Load(game.Save()) && copy.Potions()==11 && copy.ShopStock(0)==0,"Purchased potions and depleted stock must survive loading.");
  const auto soldOut=game.Save(); Buy(game,0); Require(game.Save()==soldOut,"Loading or retrying must not restock supplies.");
  Buy(game,2); const auto poor=game.Save(); Buy(game,1);
  Require(game.Gold()==19 && game.Save()==poor,"Insufficient gold must preserve the remaining stock and inventory.");
  // Reject forged receipts, refunds and gear before replacing live state.
  auto bad=bought; auto at=bad.find("SHOP 0 1 1"); Require(at!=std::string::npos,"Expected purchase record.");
  bad.replace(at,10,"SHOP 0 0 1"); const auto stable=copy.Save();
  Require(!copy.Load(bad) && copy.Save()==stable,"Missing purchase receipt must reject a forged refund/ownership state.");
  bad=soldOut; at=bad.find("SHOP 6 0 0"); bad.replace(at,10,"SHOP 7 0 0");
  Require(!copy.Load(bad) && copy.Save()==stable,"Stock purchases above the initial limit must reject transactionally.");
  // Actual old v4 layout has four gear slots and no SHOP record.
  Require(game.Load(Veteran()),"Prepare version-4 migration."); auto old=game.Save(); old.replace(14,1,"4");
  at=old.find("SHOP "); old.erase(at,old.find('\n',at)-at+1);
  at=old.find("GEAR "); const auto end=old.find('\n',at); old.erase(end-6,6);
  Require(copy.Load(old) && copy.Items().Worn(GearSlot::Head)==Item::None && copy.ShopStock(1)==1 && copy.Gold()==90,
    "Version-4 migration must retain old bag/gear without granting new equipment or charging gold.");
  auto onCounter=old; at=onCounter.find("3 -10 140"); onCounter.replace(at,9,"3 -13.3 140");
  Require(copy.Load(onCounter) && copy.Map().CanStand(copy.Player(),.4f) && copy.Gold()==90,
    "Old saves on the newly solid counter must migrate to its free edge without losing progress.");
  auto currentCounter=game.Save(); at=currentCounter.find("3 -10 140"); currentCounter.replace(at,9,"3 -13.3 140");
  const auto migrated=copy.Save();
  Require(!copy.Load(currentCounter) && copy.Save()==migrated,"Current saves inside solid geometry must still reject transactionally.");
  c={}; c.jump=true; c.purchase=1; game.Update(.01f,c);
  Require(game.Height()>0 && game.Gold()==90 && game.ShopStock(1)==1 && !game.CanTrade(),
    "Jumping must reject purchases before charging gold or adding equipment.");
  auto dead=bought; at=dead.find("140"); dead.replace(at,3,"0"); Require(copy.Load(dead),"Purchased equipment death fixture must load.");
  const auto defeated=copy.Save(); Buy(copy,0);
  Require(copy.Save()==defeated && !copy.CanTrade(),"Defeated characters cannot buy supplies.");
  c={}; c.respawn=true; copy.Update(.01f,c);
  Require(copy.Gold()==35 && copy.ShopStock(1)==0 && copy.ShopStock(2)==0 && copy.Items().Count(Item::Shield)==1,
    "Respawn must preserve spent gold, sold stock and purchased equipment.");
  c={}; c.restart=true; copy.Update(.01f,c);
  Require(copy.Gold()==0 && copy.ShopStock(1)==1 && copy.Items().Count(Item::Helm)==0,"New expedition must reset purchases and stock.");
  // New recruit can reach the vendor but cannot buy patrol-restricted gear.
  for (int i=0;i<10;++i) { c={}; c.movement={1,0}; copy.Update(.05f,c); }
  Require(copy.NearQuartermaster(),"Recruit should be within trading range.");
  Require(std::string(copy.PurchaseStatus(1))=="Complete the frontier patrol","Gear must require the actual quest hand-in.");
  const auto recruit=copy.Save(); Buy(copy,1); Require(copy.Save()==recruit,"Locked gear cannot be purchased by direct command.");
  std::cout<<"PASS: limited stock, gold, purchase validation, six-slot gear, training effects and v4/v5 migration.\n";
}
