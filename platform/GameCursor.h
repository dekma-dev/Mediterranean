#pragma once
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Owned native cursor, generated locally. No files or system cursor settings.
inline HCURSOR CreateGameCursor(UINT dpi) {
  const int size=MulDiv(32,int(std::clamp(dpi,96u,384u)),96);
  BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth=size; info.bmiHeader.biHeight=-size;
  info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
  void* bits=nullptr;
  HBITMAP color=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&bits,nullptr,0);
  if (!color) return nullptr;
  struct Point { float x,y; };
  constexpr Point outline[]={{3,2},{26,20},{17,20},{22,29},{17,31},{12,22},{5,27}};
  auto* pixels=static_cast<std::uint32_t*>(bits);
  for (int y=0;y<size;++y) for (int x=0;x<size;++x) {
    unsigned red=0,green=0,blue=0,alpha=0;
    for (int sy=0;sy<4;++sy) for (int sx=0;sx<4;++sx) {
      const Point p{(x+(sx+.5f)/4)*32/size,(y+(sy+.5f)/4)*32/size};
      bool inside=false; float edge=100;
      for (int i=0,j=6;i<7;j=i++) {
        const auto a=outline[i],b=outline[j];
        if ((a.y>p.y)!=(b.y>p.y) && p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
        const float dx=b.x-a.x,dy=b.y-a.y;
        const float t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/(dx*dx+dy*dy),0.f,1.f);
        edge=std::min(edge,std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy));
      }
      if (!inside) continue;
      alpha+=255;
      if (edge<.65f) { red+=24; green+=25; blue+=15; }
      else if (edge<2.f) { red+=236; green+=197; blue+=100; }
      else { red+=unsigned(52+p.y); green+=unsigned(105+p.y); blue+=48; }
    }
    // Premultiplied BGRA with 4x4 coverage for clean DPI-scaled edges.
    pixels[y*size+x]=(alpha/16<<24)|(red/16<<16)|(green/16<<8)|(blue/16);
  }
  std::vector<unsigned char> maskBits(size*((size+15)/16)*2,0);
  HBITMAP mask=CreateBitmap(size,size,1,1,maskBits.data());
  ICONINFO cursor{}; cursor.fIcon=FALSE;
  cursor.xHotspot=MulDiv(3,size,32); cursor.yHotspot=MulDiv(2,size,32);
  cursor.hbmColor=color; cursor.hbmMask=mask;
  HCURSOR result=mask?static_cast<HCURSOR>(CreateIconIndirect(&cursor)):nullptr;
  if (mask) DeleteObject(mask);
  DeleteObject(color);
  return result;
}
