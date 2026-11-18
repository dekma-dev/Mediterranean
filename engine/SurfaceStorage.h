#pragma once
#include <cstddef>

namespace visual {
// Keep imported 2K materials and authored 1K roof/fur fields at native size.
// Array layers: 2K = stone/wood/plaster/bark; 1K = roof/fur.
constexpr unsigned SurfaceSize(unsigned group){return group==0?2048:1024;}
constexpr unsigned SurfaceLayers(unsigned group){return group==0?4:2;}
constexpr unsigned SurfaceSource(unsigned group,unsigned layer){return group==0?(layer<3?layer:4):(layer==0?3:5);}
constexpr std::size_t SurfaceStorageBytes(){
  std::size_t bytes=0;
  for(unsigned group=0;group<2;++group)for(unsigned n=SurfaceSize(group);n;n/=2)
    bytes+=std::size_t(n)*n*4*SurfaceLayers(group)*2;
  return bytes;
}
}
