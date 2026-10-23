#include "engine/BenchmarkStats.h"
#include "engine/GpuObservation.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

void Require(bool condition,const char* message) {
  if(!condition) { std::cerr<<message<<'\n'; std::exit(1); }
}
int main() {
  std::array<GpuObservation,4> ready{{{105,5},{102,2},{99,9},{104,4}}};
  std::vector<double> gpu;
  AppendGpuObservations(gpu,ready,4,100);
  Require(gpu==std::vector<double>({5,2,4}),"Keep all completed measurements; exclude late warmup across ring wrap");
  AppendGpuObservations(gpu,ready,0,100);
  Require(gpu.size()==3,"An empty completion batch must not repeat stale samples");
  gpu.clear();AppendGpuObservations(gpu,ready,4,106);
  Require(gpu.empty(),"Previous workload results must not enter a new measured interval");
  std::ostringstream empty;
  diagnostics::WriteDistribution(empty,"GPU",{});
  Require(empty.str().find("timing unavailable")!=std::string::npos,"Missing GPU data must not become zero timing");
  Require(empty.str().find("p50")==std::string::npos,"Missing distribution must not claim percentiles");
  std::vector<double> values;
  for(int i=100;i>=1;--i) values.push_back(double(i));
  std::ostringstream report;
  diagnostics::WriteDistribution(report,"CPU",values);
  Require(report.str().find("50 / 95 / 99 / 100")!=std::string::npos,"Tail ranks must include the 99th percentile");
  Require(report.str().find("above 16.667 ms: 84")!=std::string::npos,"Budget count must include every slow sample");
  std::ostringstream single;
  diagnostics::WriteDistribution(single,"CPU",{0});
  Require(single.str().find("0 / 0 / 0 / 0")!=std::string::npos,"Measured zero remains a valid measurement");
}
