#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class Renderer {
public:
  bool Initialize(HWND hwnd, int width, int height);
  void BeginFrame();
  void EndFrame();

private:
  void CreateRenderTarget();

  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
  D3D11_VIEWPORT viewport_ = {};
  float clearColor_[4] = { 0.08f, 0.1f, 0.12f, 1.0f };
};