#include "engine/SurfaceStorage.h"
#include <array>
#include <cstdlib>
#include <iostream>

void Check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
  std::array<unsigned,6> occurrences{};
  for(unsigned group=0;group<2;++group)for(unsigned layer=0;layer<visual::SurfaceLayers(group);++layer){
    const auto source=visual::SurfaceSource(group,layer);
    Check(source<6,"Surface layer exceeds its manifest");++occurrences[source];
    Check(visual::SurfaceSize(group)==((source==3||source==5)?1024u:2048u),"A retained surface is enlarged or a 2K original is reduced");
  }
  for(auto count:occurrences)Check(count==1,"Surface grouping lost or duplicated a material");
  Check(visual::SurfaceStorageBytes()==201326576,"Native-size color/normal mip chains exceeded their exact storage budget");
  std::cout<<"PASS: all six color/normal surfaces retain native dimensions, unique layer mappings and bounded mip storage\n";
}
