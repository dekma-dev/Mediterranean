#include "engine/Audio.h"
#include <algorithm>
#include <cmath>

namespace sound {
void DeliveryQueue::Advance(float seconds) {
  if(!std::isfinite(seconds) || seconds<=0)return;
  unsigned retained=0;
  for(unsigned i=0;i<count_;++i){auto entry=pending_[i];entry.age+=seconds;
    if(entry.age>maximumAge)++stats_.expired;else pending_[retained++]=entry;}
  count_=retained;
}
void DeliveryQueue::Append(const Events& events) {
  stats_.dropped+=events.dropped;
  for(unsigned i=0;i<std::min<unsigned>(events.count,unsigned(events.cues.size()));++i){
    const Entry incoming{events.cues[i],events.origins[i],events.heights[i],0,events.spatial[i],events.environmental[i],events.attachedActors[i],events.preparationTokens[i],events.emitterIdentities[i]};
    if(unsigned(incoming.cue)>=unsigned(Cue::Count)){++stats_.dropped;continue;}
    if(count_==capacity) {
      unsigned lowest=0;for(unsigned j=1;j<count_;++j)if(CuePriority(pending_[j].cue)<CuePriority(pending_[lowest].cue))lowest=j;
      ++stats_.dropped;
      if(CuePriority(incoming.cue)<CuePriority(pending_[lowest].cue))continue;
      std::move(pending_.begin()+lowest+1,pending_.begin()+count_,pending_.begin()+lowest);--count_;
    }
    pending_[count_++]=incoming;++stats_.accepted;stats_.peak=std::max(stats_.peak,count_);
  }
}
Events DeliveryQueue::Take() {
  Events result;
  std::array<bool,capacity> keep{};
  const unsigned wanted=std::min<unsigned>(count_,unsigned(result.cues.size()));
  if(count_<=wanted)std::fill_n(keep.begin(),count_,true);
  else {
    // Prefer combat warnings/contacts over incidental footsteps on overload,
    // then deliver selected entries in their original chronological order.
    for(unsigned n=0;n<wanted;++n){unsigned best=capacity;
      for(unsigned i=0;i<count_;++i)if(!keep[i] && (best==capacity || CuePriority(pending_[i].cue)>CuePriority(pending_[best].cue)))best=i;
      keep[best]=true;}
    stats_.dropped+=count_-wanted;
  }
  for(unsigned i=0;i<count_;++i)if(keep[i]){const auto& entry=pending_[i];const auto j=result.count++;
    result.cues[j]=entry.cue;result.origins[j]=entry.origin;result.heights[j]=entry.height;
    result.spatial[j]=entry.spatial;result.environmental[j]=entry.environmental;result.attachedActors[j]=entry.actor;
    result.preparationTokens[j]=entry.preparationToken;result.emitterIdentities[j]=entry.identity;}
  stats_.delivered+=result.count;count_=0;return result;
}
}
