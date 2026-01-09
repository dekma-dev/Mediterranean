#include "engine/Renderer.h"

#include <dxgi.h>

bool Renderer::Initialize(HWND hwnd, int width, int height) {
  DXGI_SWAP_CHAIN_DESC swapDesc = {};
  swapDesc.BufferCount = 2;
  swapDesc.BufferDesc.Width = width;
  swapDesc.BufferDesc.Height = height;
  swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapDesc.OutputWindow = hwnd;
  swapDesc.SampleDesc.Count = 1;
  swapDesc.Windowed = TRUE;
  swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT deviceFlags = 0;
#ifdef _DEBUG
  deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
  D3D_FEATURE_LEVEL createdLevel = {};

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    deviceFlags,
    featureLevels,
    1,
    D3D11_SDK_VERSION,
    &swapDesc,
    swapChain_.GetAddressOf(),
    device_.GetAddressOf(),
    &createdLevel,
    context_.GetAddressOf()
  );

  if (FAILED(hr)) {
    return false;
  }

  CreateRenderTarget();

  viewport_.TopLeftX = 0.0f;
  viewport_.TopLeftY = 0.0f;
  viewport_.Width = static_cast<float>(width);
  viewport_.Height = static_cast<float>(height);
  viewport_.MinDepth = 0.0f;
  viewport_.MaxDepth = 1.0f;
  context_->RSSetViewports(1, &viewport_);

  return true;
}

void Renderer::BeginFrame() {
  if (!context_) return;
  context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);
  context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor_);
}

void Renderer::EndFrame() {
  if (!swapChain_) return;
  swapChain_->Present(1, 0);
}

void Renderer::CreateRenderTarget() {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
  swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
  device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView_.GetAddressOf());
}