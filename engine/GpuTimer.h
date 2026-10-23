#pragma once
#include <array>
#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>
#include "engine/GpuObservation.h"

// Asynchronous timestamps: unavailable samples are skipped, never waited on.
class GpuTimer {
public:
  bool Initialize(ID3D11Device* device);
  void Begin(ID3D11DeviceContext* context);
  void End(ID3D11DeviceContext* context);
  double Milliseconds() const { return milliseconds_; }
  std::uint64_t Samples() const { return samples_; }
  std::uint64_t Frame() const { return frame_; }
  const std::array<GpuObservation,4>& Ready() const { return ready_; }
  std::size_t ReadyCount() const { return readyCount_; }
private:
  struct Slot {
    Microsoft::WRL::ComPtr<ID3D11Query> disjoint,begin,end;
    bool pending = false;
    std::uint64_t frame=0;
  };
  std::array<Slot,4> slots_;
  int current_ = 0, active_ = -1;
  double milliseconds_ = 0;
  std::uint64_t samples_ = 0;
  std::uint64_t frame_=0,latestFrame_=0;
  std::array<GpuObservation,4> ready_{};
  std::size_t readyCount_=0;
};
