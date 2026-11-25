#pragma once
#include <dxgi.h>
#include <array>
#include <cstdint>

namespace graphics {
// Invalid commands and allocation failures need diagnosis, not a retry loop.
inline bool DeviceLost(HRESULT failure) {
  return failure==DXGI_ERROR_DEVICE_REMOVED || failure==DXGI_ERROR_DEVICE_RESET ||
    failure==DXGI_ERROR_DEVICE_HUNG || failure==DXGI_ERROR_DRIVER_INTERNAL_ERROR;
}
inline bool Recoverable(HRESULT failure,HRESULT reason) {
  return reason!=DXGI_ERROR_INVALID_CALL && (DeviceLost(failure) || DeviceLost(reason));
}
class RecoveryBudget {
public:
  // At most two device reconstructions in any rolling minute, including
  // failures that recur immediately after a superficially successful rebuild.
  bool Request(std::uint64_t now) {
    unsigned retained=0;
    for(unsigned i=0;i<count_;++i)
      if(now<attempts_[i] || now-attempts_[i]<60000) attempts_[retained++]=attempts_[i];
    count_=retained;
    if(count_==attempts_.size()) return false;
    attempts_[count_++]=now; return true;
  }
private:
  std::array<std::uint64_t,2> attempts_{};
  unsigned count_=0;
};
}
