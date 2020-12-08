# Anti-Aliasing

Anti-Aliasing is a Windows 10 desktop application with implementations of several different anti-aliasing methods.

# Installation

To run this project you need Micosoft Visual Studio 2019 with the "Desktop development with C++" option and the latest Windows SDK.

# Description

`ELib/graphics/`: Everything related to DirectX 12

`ELib/math/`                    : Some helper classes for math

`ELib/window/`                  : Everything related to Windows

`ELib/misc/`                    : Helper functions and classes for various purposes

`Rendering/deferred_rendering`  : Classes used for deferred rendering

`Rendering/ray_tracer`          : Class for using ray tracing

`Rendering/models`              : .obj files for meshes used

`Rendering/textures`            : Texture files for mesh textures

`Rendering/aa`                  : Classes for applying anti-aliasing

`Rendering/shaders`             : Contains all shaders used in this project

`Anti-Aliasing/app.cpp`         : Main file for this project

# Usage

| Keyboard & mouse input | Action |
|--- | --- |
| WASD | Movement control |
| Mouse movement | Rotation |
| 1 | Set Anti-aliasing mode off |
| 2 | Set Anti-aliasing mode to FXAA |
| 3 | Set Anti-aliasing mode to TAA |
| 4 | Set Anti-aliasing mode to SSAA |
| 5 | Toggle freeze frame (The right arrow button can be used to progress frames when the frame is frozen) |
| 6 | Toggle ray tracing |

*When in TAA mode:*

| Keyboard & mouse input | Action |
|--- | --- |
| R | Toggle between bilinear interpolation and bicubic interpolation |
| T | Toggle history rectification |
| Y | Toggle between YCoCg space and RGB space |
| U | Toggle between history clipping and history clamping |


Temporal upsampling is used by changing the constants `upsample_numerator` and `upsample_denominator` in `Anti-Aliasing/app.cpp` at lines 19 and 20
