#include "tools/GltfRigImport.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int wmain(int argc,wchar_t** argv) {
  if(argc!=2) {std::cerr<<"Usage: RigAssetInspector <rig.glb>\n";return 2;}
  try {
    const std::filesystem::path path(argv[1]);std::error_code error;
    const auto length=std::filesystem::file_size(path,error);
    if(error || length<20 || length>4*1024*1024) throw std::runtime_error("Rig file must be 20 bytes..4 MiB");
    std::ifstream input(path,std::ios::binary);std::string bytes(std::size_t(length),'\0');
    if(!input.read(bytes.data(),std::streamsize(bytes.size())) || input.peek()!=std::char_traits<char>::eof())
      throw std::runtime_error("Cannot read stable rig file");
    const auto asset=assets::ImportGlbAsset(bytes);const auto& skeleton=asset.skeleton;const auto pose=skeleton.RestPose();
    std::cout<<"Validated rig joints: "<<skeleton.Size()<<"\nEngine space: left-handed, metres, Y up, +Z forward\n";
    for(std::size_t i=0;i<pose.global.size();++i) {
      const auto& m=pose.global[i];std::cout<<"Joint "<<i<<" position: "<<m._41<<' '<<m._42<<' '<<m._43<<'\n';
    }
    std::cout<<"Validated clips: "<<asset.clips.size()<<'\n';
    for(const auto& named:asset.clips) {
      const auto midpoint=skeleton.Sample(named.clip,named.clip.duration*.5,false);
      std::cout<<"Clip "<<named.name<<" duration: "<<named.clip.duration<<" s; midpoint joint 0: "
        <<midpoint.global[0]._41<<' '<<midpoint.global[0]._42<<' '<<midpoint.global[0]._43<<'\n';
    }
    std::cout<<"Rig/clip validation; no mesh or GPU integration. Source unchanged.\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
