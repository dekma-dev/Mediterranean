#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <vector>
class Renderer {
public:
  bool Initialize(HWND window, int width, int height);
  bool Resize(int width, int height);
  void BeginFrame();
  bool EndFrame();
  void SetCamera(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target);
  void DrawBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color, float yaw = 0);
  void DrawPyramid(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale,
    const DirectX::XMFLOAT4& color, float yaw = 0, float pitch = 0);
  bool Capture(const std::wstring& path);
  UINT DrawCalls() const { return drawCalls_ + static_cast<UINT>((hud_.size()*6+16379)/16380); }
  void DrawHudRect(float x, float y, float width, float height, const DirectX::XMFLOAT4& color);
  void DrawText(float x, float y, const std::string& text, const DirectX::XMFLOAT4& color, float size = 2);
  const std::wstring& Error() const { return error_; }
private:
  bool Check(HRESULT result, const wchar_t* operation);
  bool CreateTargets(int width, int height);
  bool CreatePipeline();
  bool FlushHud();
  struct HudRect { float x, y, width, height; DirectX::XMFLOAT4 color; };
  std::vector<HudRect> hud_;
  void Draw(const DirectX::XMMATRIX& world, const DirectX::XMMATRIX& vp,
    const DirectX::XMFLOAT4& color, bool hud, UINT count = 36, UINT start = 0);
  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
  Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_, constants_, hudVertices_;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthOn_, depthOff_;
  DirectX::XMFLOAT4X4 viewProjection_{};
  DirectX::XMFLOAT3 eye_{};
  int width_ = 1, height_ = 1;
  UINT drawCalls_ = 0;
  std::wstring error_;
};
