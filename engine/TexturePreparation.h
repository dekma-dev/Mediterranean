#pragma once
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace resources {
enum class TextureKind { Surface, Interface, Effect };
struct TextureRequest {
  TextureKind kind=TextureKind::Surface;
  unsigned slot=0,layer=0,resource=0,width=0,height=0;
  bool scale=false;
  std::size_t Bytes()const{return std::size_t(width)*height*4;}
  bool operator==(const TextureRequest& other)const;
};
std::vector<TextureRequest> TextureManifest();

struct TexturePreparationStats {
  std::size_t liveBytes=0,peakBytes=0;
  unsigned livePackets=0,peakPackets=0,prepared=0,delivered=0;
  DWORD workerThread=0;
  double decodeSeconds=0,producerWaitSeconds=0,consumerWaitSeconds=0;
};
struct TextureBudget;
// A packet holds its reservation through GPU upload, not just through dequeue.
// It may outlive its producer; releasing it never touches a renderer or device.
class PreparedTexture {
public:
  PreparedTexture()=default;
  ~PreparedTexture();
  PreparedTexture(PreparedTexture&& other)noexcept;
  PreparedTexture& operator=(PreparedTexture&& other)noexcept;
  PreparedTexture(const PreparedTexture&)=delete;
  PreparedTexture& operator=(const PreparedTexture&)=delete;
  const TextureRequest& Request()const{return request_;}
  const std::uint8_t* Pixels()const{return pixels_.get();}
  std::size_t Bytes()const{return pixels_?request_.Bytes():0;}
  void Reset()noexcept;
private:
  friend class TexturePreparation;
  TextureRequest request_{};
  std::unique_ptr<std::uint8_t[]> pixels_;
  std::shared_ptr<TextureBudget> budget_;
};

// One CPU worker, at most two reserved packets, 32 MiB by default including
// decoding, queued and consumer-held pixels. WIC private scratch is separate.
// Start/Take/destruction belong to the consumer; Cancel may run on any thread.
// No COM interfaces, GPU objects or callbacks into Renderer cross the boundary.
class TexturePreparation {
public:
  static constexpr std::size_t defaultBudget=32u*1024u*1024u;
  using Decoder=std::function<HRESULT(const TextureRequest&,std::uint8_t*,const std::atomic<bool>&)>;
  enum class Result { Ready, Pending, Finished, Cancelled, Failed };
  explicit TexturePreparation(std::vector<TextureRequest> requests=TextureManifest(),
    std::size_t byteBudget=defaultBudget,Decoder decoder={});
  ~TexturePreparation();
  TexturePreparation(const TexturePreparation&)=delete;
  TexturePreparation& operator=(const TexturePreparation&)=delete;
  Result Take(PreparedTexture& result,std::chrono::milliseconds wait=std::chrono::milliseconds(16));
  void Cancel()noexcept;
  // Consumer-only join/drain; idempotent. Retained consumer packets stay valid.
  void Stop();
  TexturePreparationStats Stats()const;
  HRESULT FailureCode()const;
  std::wstring Error()const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
