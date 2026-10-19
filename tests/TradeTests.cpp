#include "game/Simulation.h"
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <sstream>

void Require(bool value,const char* message) { if (!value) { std::cerr<<message<<'\n'; std::exit(1); } }
std::string Veteran() {
  return "MEDITERRANEAN 3\n3 -10 140 0 3 90\n5 1 1 0\n4\n0 0 0 0\n"
    "100 -4 7 0 0 1 3 0 0\n101 4 9 0 0 1 3 0 0\n102 0 12 0 0 1 3 0 0\n";
}
void Buy(rpg::Simulation& game,int index) { rpg::Command c; c.purchase=index; game.Update(.01f,c); }
void Trade(rpg::Simulation& game,rpg::TradeAction action,rpg::Item item,int source=-1) {
  rpg::Command c; c.trade={action,item,source}; game.Update(.01f,c);
}
void RemoveRecord(std::string& save,const char* tag) {
  const auto at=save.find(tag); Require(at!=std::string::npos,"Expected save record."); save.erase(at,save.find('\n',at)-at+1);
}
void TrimRecord(std::string& save,const char* tag,int count) {
  const auto at=save.find(tag); Require(at!=std::string::npos,"Expected legacy record.");
  const auto end=save.find('\n',at); std::istringstream input(save.substr(at,end-at));
  std::string name; input>>name; std::ostringstream output; output<<name;
  for (int i=0;i<count;++i) { int value; Require(bool(input>>value),"Expected legacy field."); output<<' '<<value; }
  save.replace(at,end-at,output.str());
}
int main() {
  using namespace rpg;
  {
    Simulation dash; Require(dash.Load(Veteran()),"Dash shop fixture must load");
    Command input; input.ability=2; input.purchase=1; dash.Update(.01f,input);
    Require(dash.Dashing() && dash.Gold()==90 && dash.ShopStock(1)==1 &&
      dash.Items().Count(Item::Helm)==0,
      "Dash activation must reject a simultaneous purchase before changing gold or stock");
  }
  {
    Simulation rebound; Require(rebound.Load(Veteran()),"Rebound shop fixture must load");
    for(int i=0;i<11;++i) { Command jump; jump.jump=i==0 || i==10; rebound.Update(.05f,jump); }
    const float speed=rebound.VerticalSpeed();
    const float contact=2*rebound.Height()/(std::sqrt(speed*speed+48*rebound.Height())-speed);
    Command buy; buy.purchase=1; rebound.Update(contact,buy);
    Require(rebound.Height()==0 && rebound.VerticalSpeed()>0,"Shop fixture must end exactly at rebound takeoff");
    Require(!rebound.CanTrade() && rebound.Gold()==90 && rebound.Items().Count(Item::Helm)==0,
      "A zero-height rising rebound must not permit shopping");
  }
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
    "Current saves must retain purchases, exact gold, stock and equipment.");
  const auto armored=game.Save();
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
  // Actual v6 has nine buyback counts and six worn slots; migration adds travel gear once.
  auto version6=armored; version6.replace(14,1,"6"); TrimRecord(version6,"GEAR ",6); TrimRecord(version6,"BUYBACK ",9);
  Require(copy.Load(version6) && copy.Save()==armored,"Version-6 migration must preserve old gear/ledger and add starting travel pieces once.");
  // Actual v5 has purchases and six gear slots but no buyback record.
  auto version5=armored; version5.replace(14,1,"5"); RemoveRecord(version5,"BUYBACK "); TrimRecord(version5,"GEAR ",6);
  Require(copy.Load(version5) && copy.Save()==armored,"Version-5 migration must preserve purchases/worn six-slot gear and start with empty buyback.");
  // Actual old v4 layout has four gear slots and no SHOP/BUYBACK record.
  Require(game.Load(Veteran()),"Prepare version-4 migration."); auto old=game.Save(); old.replace(14,1,"4");
  at=old.find("SHOP "); old.erase(at,old.find('\n',at)-at+1);
  RemoveRecord(old,"BUYBACK ");
  TrimRecord(old,"GEAR ",4);
  Require(copy.Load(old) && copy.Items().Worn(GearSlot::Head)==Item::None && copy.ShopStock(1)==1 && copy.Gold()==90,
    "Version-4 migration must retain old bag/gear while adding travel gear without granting shop equipment or charging gold.");
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
  for (int i=0;i<10;++i) copy.Update(.05f,{}); // Finish physical braking before checking transaction atomicity.
  Require(copy.NearQuartermaster(),"Recruit should be within trading range.");
  Require(std::string(copy.PurchaseStatus(1))=="Complete the frontier patrol","Gear must require the actual quest hand-in.");
  const auto recruit=copy.Save(); Buy(copy,1); Require(copy.Save()==recruit,"Locked gear cannot be purchased by direct command.");
  // Sold items remain held by the merchant, and their value reconciles with gold.
  Require(game.Load(Veteran()),"Prepare selling scenario.");
  const auto beforeSales=game.Save();
  Trade(game,TradeAction::Sell,Item::Watchguard,-1);
  Trade(game,TradeAction::Sell,Item::Starter,game.Items().Find(Item::Potion));
  Trade(game,TradeAction::Buyback,Item::Shield);
  Trade(game,TradeAction::Sell,Item::Count,0);
  Require(game.Save()==beforeSales,"Worn, stale, missing and unknown trades must be atomic rejections.");
  for (auto item : {Item::Starter,Item::Tunic}) Trade(game,TradeAction::Sell,item,game.Items().Find(item));
  Require(game.Gold()==97 && game.Items().Count(Item::Starter)==0 && game.BuybackCount(Item::Starter)==1,
    "Sale must move gear to merchant ownership and permit legitimate gold above the old 90 cap.");
  c={}; c.inventory={InventoryAction::Unequip,int(GearSlot::Weapon),-1,Item::Watchguard}; game.Update(.01f,c);
  Trade(game,TradeAction::Sell,Item::Watchguard,game.Items().Find(Item::Watchguard));
  c={}; c.inventory={InventoryAction::Unequip,int(GearSlot::Chest),-1,Item::Ward}; game.Update(.01f,c);
  Trade(game,TradeAction::Sell,Item::Ward,game.Items().Find(Item::Ward));
  c={}; c.equipWeapon=true; game.Update(.01f,c);
  Require(game.Experience()==300 && game.Level()==3 && !game.HasRewardWeapon() && game.StrikeDamage()==12 && game.DamageReduction()==0,
    "Selling quest gear must retain earned XP; G cannot equip a sold sword.");
  Require(copy.Load(game.Save()) && copy.Save()==game.Save(),"Sold rewards and starting gear must survive a version-7 save.");
  const auto soldGear=game.Save();
  auto soldV6=soldGear; soldV6.replace(14,1,"6"); TrimRecord(soldV6,"GEAR ",6); TrimRecord(soldV6,"BUYBACK ",9);
  Require(copy.Load(soldV6) && copy.Save()==soldGear,
    "Version-6 migration must preserve merchant-held rewards and sale credits alongside new travel gear.");
  for (int i=0;i<5;++i) Trade(game,TradeAction::Sell,Item::Potion,game.Items().Find(Item::Potion));
  Require(game.Potions()==0 && game.BuybackCount(Item::Potion)==5 && game.Gold()==132,"Each potion sale transfers one, including the last item in its cell.");
  Require(copy.Load(game.Save()) && copy.Save()==game.Save(),"Potion buyback counts must persist even with no potions left in the bag.");
  for (int i=0;i<5;++i) Trade(game,TradeAction::Buyback,Item::Potion);
  Require(game.Save()==soldGear,"Selling and reclaiming the same stack must not create gold or potions.");
  for (auto item : {Item::Starter,Item::Tunic,Item::Watchguard,Item::Ward}) Trade(game,TradeAction::Buyback,item);
  Require(game.Gold()==90 && game.Items().Count(Item::Ward)==1 && !game.WardArmorEquipped(),"Reclaimed equipment returns to bag at exactly its sale price.");
  Require(copy.Load(game.Save()),"Fully reclaimed inventory must remain valid.");
  // Spend sale proceeds: buyback may become unaffordable but may never disappear.
  Require(game.Load(Veteran()),"Prepare spending buyback scenario."); Buy(game,1); Buy(game,2);
  Trade(game,TradeAction::Sell,Item::Shield,game.Items().Find(Item::Shield));
  for (int i=0;i<6;++i) Buy(game,0);
  Require(game.Gold()==16 && game.BuybackCount(Item::Shield)==1 && game.ShopStock(2)==0,"Selling must not replenish the original shop stock.");
  const auto noGold=game.Save(); Trade(game,TradeAction::Buyback,Item::Shield);
  Require(game.Save()==noGold && std::string(game.TradeStatus({TradeAction::Buyback,Item::Shield}))=="Not enough gold",
    "Unaffordable buyback must retain the item, gold and inventory.");
  Trade(game,TradeAction::Sell,Item::Potion,game.Items().Find(Item::Potion)); Trade(game,TradeAction::Buyback,Item::Shield);
  Require(game.Gold()==1 && game.BuybackCount(Item::Shield)==0 && game.Items().Count(Item::Shield)==1 && copy.Load(game.Save()),
    "Further sales may fund reclaiming without resetting stock or violating the ledger.");
  // Reject manipulated buyback counts/credits and duplicate unique ownership.
  const auto valid=copy.Save();
  const auto reject=[&](std::string data) { Require(!copy.Load(data) && copy.Save()==valid,"Forged buyback save must reject transactionally."); };
  bad=soldGear; at=bad.find("BUYBACK "); bad.replace(at,bad.find('\n',at)-at,"BUYBACK 1 1 0 1 0 0 1 1 0 0 0 0 0"); reject(bad);
  bad=soldGear; at=bad.find("BUYBACK "); bad.replace(at,bad.find('\n',at)-at,"BUYBACK 1 1 12 1 0 0 1 0 0 0 0 0 0"); reject(bad);
  bad=soldGear; at=bad.find("BUYBACK "); bad.replace(at,bad.find('\n',at)-at,"BUYBACK -1 1 0 1 0 0 1 0 0 0 0 0 0"); reject(bad);
  bad=soldGear; at=bad.find("3 -10 140 0 3 122"); Require(at!=std::string::npos,"Expected sale balance.");
  bad.replace(at,17,"3 -10 140 0 3 123"); reject(bad);
  bad=soldGear; at=bad.find(" -1 0",bad.find("BAG")); bad.replace(at,5," 0 1"); reject(bad);
  // Even internally balanced gold cannot authorize an unearned item.
  bad=soldGear; at=bad.find("BUYBACK "); bad.replace(at,bad.find('\n',at)-at,"BUYBACK 1 1 0 1 0 0 1 1 0 0 0 0 0");
  at=bad.find("3 -10 140 0 3 122"); bad.replace(at,17,"3 -10 140 0 3 132"); reject(bad);
  // Reclaim and sell every catalog item repeatedly; unique gear never returns to retail stock.
  Require(game.Load(armored),"Prepare catalog round trip.");
  for (int i=0;i<int(Item::Count);++i) {
    const auto item=Item(i); const auto slot=DescribeItem(item).slot;
    if (slot!=GearSlot::None && game.Items().Worn(slot)==item) {
      c={}; c.inventory={InventoryAction::Unequip,int(slot),-1,item}; game.Update(.01f,c);
    }
    const int beforeGold=game.Gold(),beforeCount=game.Items().Count(item);
    for (int cycle=0;cycle<12;++cycle) {
      Trade(game,TradeAction::Sell,item,game.Items().Find(item));
      Require(game.BuybackCount(item)==1 && game.Gold()==beforeGold+SellPrice(item),"Catalog sale transfers one item at its price.");
      if (cycle==0) Require(copy.Load(game.Save()) && copy.Save()==game.Save(),
        "Merchant-held catalog items, including travel gear, must persist without reappearing on the player.");
      Trade(game,TradeAction::Buyback,item);
      Require(game.BuybackCount(item)==0 && game.Gold()==beforeGold && game.Items().Count(item)==beforeCount,
        "Repeated catalog trades cannot farm gold or duplicate items.");
    }
    Require(copy.Load(game.Save()) && game.Experience()==300,"Every catalog item's transfer must remain save-valid without changing XP.");
  }
  // Eligibility applies to sales and buyback as well as fresh purchases.
  Require(game.Load(soldGear),"Prepare trade eligibility scenario.");
  c={}; c.jump=true; c.trade={TradeAction::Buyback,Item::Starter}; game.Update(.01f,c);
  Require(game.Gold()==122 && game.BuybackCount(Item::Starter)==1,"Jumping must reject buyback before charging.");
  Require(game.Load(soldGear),"Reset remote trade scenario.");
  for (int i=0;i<30;++i) { c={}; c.movement={0,1}; game.Update(.05f,c); }
  for (int i=0;i<10;++i) game.Update(.05f,{});
  const auto away=game.Save(); Trade(game,TradeAction::Sell,Item::Potion,game.Items().Find(Item::Potion)); Trade(game,TradeAction::Buyback,Item::Starter);
  Require(!game.CanTrade() && game.Save()==away,"Remote sale and buyback must reject without side effects.");
  bad=soldGear; at=bad.find("140"); bad.replace(at,3,"0"); Require(game.Load(bad),"Sold-item death fixture.");
  const auto deadSales=game.Save(); Trade(game,TradeAction::Sell,Item::Potion,game.Items().Find(Item::Potion)); Trade(game,TradeAction::Buyback,Item::Starter);
  Require(game.Save()==deadSales,"Defeated characters cannot sell or reclaim.");
  c={}; c.respawn=true; game.Update(.01f,c);
  Require(game.Gold()==122 && game.BuybackCount(Item::Ward)==1 && copy.Load(game.Save()),"Respawn must preserve sold gear and sale proceeds.");
  c={}; c.restart=true; game.Update(.01f,c);
  Require(game.Gold()==0 && game.BuybackCount(Item::Ward)==0 && game.Items().Count(Item::Starter)==1,"Restart resets buyback and restores starting gear.");
  std::cout<<"PASS: stock, selling, buyback, exact gold/item accounting, eligibility, equipment effects and v4-v7 migration.\n";
}
