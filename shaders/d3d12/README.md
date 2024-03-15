Compiling D3D12 Shaders for Bindless Texture support
====================================================

When using bindless textures you need to compile this shaders with DXC (DX compiler). You can either download and copy and paste the DXC and it's dependecies in this folder. Or compile it with your own DXC installation. If you do this you'll need to change the compile_shader.bat file to work with the local copy of DXC.

You can download the compiler from here: [https://github.com/microsoft/DirectXShaderCompiler/releases](https://github.com/microsoft/DirectXShaderCompiler/releases)
