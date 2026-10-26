#include "engine/FontAtlas.h"
#include <vector>
bool FontAtlas::Initialize(ID3D11Device* device) {
  // Rasterize the installed Windows font into a 256 KiB atlas. No font file is
  // copied, modified or downloaded. Grayscale AA avoids colored subpixel fringes.
  HDC dc=CreateCompatibleDC(nullptr); if (!dc) return false;
  BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth=512; info.bmiHeader.biHeight=-512;
  info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
  void* bits=nullptr; HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
  HFONT font=CreateFontW(-24,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
  if (!bitmap || !font) { if (bitmap) DeleteObject(bitmap); if (font) DeleteObject(font); DeleteDC(dc); return false; }
  auto oldBitmap=SelectObject(dc,bitmap); auto oldFont=SelectObject(dc,font);
  PatBlt(dc,0,0,512,512,BLACKNESS); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(255,255,255));
  for (int i=0;i<194;++i) {
    const wchar_t ch=static_cast<wchar_t>(i<95?i+32:i<191?0x400+i-95:i==191?0xab:i==192?0xbb:0x2212); SIZE size{};
    GetTextExtentPoint32W(dc,&ch,1,&size);
    const int x=(i%16)*32,y=(i/16)*40;
    TextOutW(dc,x+1,y+1,&ch,1); glyphs_[i]={float(x)/512,float(y)/512,float(size.cx)};
  }
  GdiFlush();
  std::vector<unsigned char> pixels(512*512);
  const auto* source=static_cast<const unsigned char*>(bits);
  for (std::size_t i=0;i<pixels.size();++i) pixels[i]=source[i*4+1];
  SelectObject(dc,oldFont); SelectObject(dc,oldBitmap); DeleteObject(font); DeleteObject(bitmap); DeleteDC(dc);
  D3D11_TEXTURE2D_DESC desc{}; desc.Width=512; desc.Height=512; desc.MipLevels=desc.ArraySize=1;
  desc.Format=DXGI_FORMAT_R8_UNORM; desc.SampleDesc.Count=1; desc.Usage=D3D11_USAGE_IMMUTABLE; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA data{pixels.data(),512,0}; Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  return SUCCEEDED(device->CreateTexture2D(&desc,&data,texture.GetAddressOf())) &&
    SUCCEEDED(device->CreateShaderResourceView(texture.Get(),nullptr,view_.GetAddressOf()));
}
