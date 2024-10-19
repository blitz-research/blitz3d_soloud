## Blitz3D open source release.

NOTE: This repository is now closed to 3rd party PR's. Unfortunately PR's can't be disabled in github, so please be aware that if you make one it will very probably be closed with no explanation, please don't be offended.

I am still updating this repository occasionally for LibSGD related changes so it will otherwise remain active for a while yet.

### Building Blitz3D from source

You will need to install Microsoft Visual Studio, and the CMake and Git utilities. Any recent version of MSVC should be OK, I am currently using Community Edition 2022.

You will also need to install the following optional MSVC components: "Desktop development with C++", "MFC and ATL support" and "ASP.NET and web development".

Then, from a DOS prompt:

``` shell
git clone https://github.com/blitz-research/blitz3d_soloud.git
cd blitz3d_soloud
cmake -S . -B cmake-build-release -A Win32 -G "Visual Studio 17 2022"
cmake --build cmake-build-release --config Release
```
Assuming all went well, the BLITZ3D_INSTALL directory will contain the final binaries, simply run Blitz3D.exe to get blitzing!

### Too lazy to build?

Grab the prebuilt version from https://blitzresearch.itch.io/
