RPG Native (DirectX 11)

Build (Visual Studio Generator)
1. Open a Developer PowerShell for VS.
2. From this folder:
   cmake -S . -B build
   cmake --build build --config Release
3. Run:
   .\build\Release\RPGNative.exe

Notes
- Windows-only prototype.
- This is a minimal engine skeleton: Win32 window, D3D11 device, clear + present loop.