#pragma once
#include <algorithm>
#include <cmath>
#include <ostream>
#include <vector>

namespace diagnostics {
// Nearest-rank percentiles. Keep missing measurements distinct from zero cost.
inline void WriteDistribution(std::ostream& out,const char* label,std::vector<double> samples) {
  out << '\n' << label << " samples: " << samples.size();
  if(samples.empty()) { out << "; timing unavailable"; return; }
  if(samples.size()<100) out << "; fewer than 100 samples: tail percentiles have coarse resolution";
  std::sort(samples.begin(),samples.end());
  const auto rank=[&](double p) { return samples[std::size_t(std::ceil(p*samples.size()))-1]; };
  const auto slow=std::count_if(samples.begin(),samples.end(),[](double ms){return ms>1000.0/60.0;});
  out << "\n" << label << " p50 / p95 / p99 / max ms: "
      << rank(.5) << " / " << rank(.95) << " / " << rank(.99) << " / " << samples.back()
      << "\n" << label << " samples above 16.667 ms: " << slow;
}
}
