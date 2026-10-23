#include "engine/GpuTimer.h"

bool GpuTimer::Initialize(ID3D11Device* device) {
  for (auto& slot : slots_) {
    D3D11_QUERY_DESC query{D3D11_QUERY_TIMESTAMP_DISJOINT,0};
    if (FAILED(device->CreateQuery(&query,slot.disjoint.GetAddressOf()))) return false;
    query.Query=D3D11_QUERY_TIMESTAMP;
    if (FAILED(device->CreateQuery(&query,slot.begin.GetAddressOf())) ||
      FAILED(device->CreateQuery(&query,slot.end.GetAddressOf()))) return false;
  }
  return true;
}
void GpuTimer::Begin(ID3D11DeviceContext* context) {
  readyCount_=0; ++frame_;
  for (auto& slot : slots_) if (slot.pending) {
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{}; UINT64 start=0,end=0;
    if (context->GetData(slot.disjoint.Get(),&disjoint,sizeof(disjoint),D3D11_ASYNC_GETDATA_DONOTFLUSH)==S_OK &&
      context->GetData(slot.begin.Get(),&start,sizeof(start),D3D11_ASYNC_GETDATA_DONOTFLUSH)==S_OK &&
      context->GetData(slot.end.Get(),&end,sizeof(end),D3D11_ASYNC_GETDATA_DONOTFLUSH)==S_OK) {
      if (!disjoint.Disjoint && disjoint.Frequency && end>=start) {
        const double milliseconds=1000.0*double(end-start)/double(disjoint.Frequency);
        ready_[readyCount_++]={slot.frame,milliseconds}; ++samples_;
        // Slots are visited in storage order, which wraps independently of age.
        if(slot.frame>latestFrame_) { latestFrame_=slot.frame; milliseconds_=milliseconds; }
      }
      slot.pending=false;
    }
  }
  active_=-1;
  auto& slot=slots_[current_];
  if (!slot.pending && slot.disjoint) {
    slot.frame=frame_;
    active_=current_; context->Begin(slot.disjoint.Get()); context->End(slot.begin.Get());
  }
}
void GpuTimer::End(ID3D11DeviceContext* context) {
  if (active_>=0) {
    auto& slot=slots_[active_];
    context->End(slot.end.Get()); context->End(slot.disjoint.Get()); slot.pending=true;
  }
  current_=(current_+1)%static_cast<int>(slots_.size()); active_=-1;
}
