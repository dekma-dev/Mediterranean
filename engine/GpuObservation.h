#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct GpuObservation { std::uint64_t frame=0; double milliseconds=0; };
// A query ring can complete several frames at once. Filter by the originating
// frame, never by when the CPU happened to poll the result.
inline void AppendGpuObservations(std::vector<double>& destination,
    const std::array<GpuObservation,4>& ready,std::size_t count,std::uint64_t firstFrame) {
  for(std::size_t i=0;i<count && i<ready.size();++i)
    if(ready[i].frame>=firstFrame) destination.push_back(ready[i].milliseconds);
}
