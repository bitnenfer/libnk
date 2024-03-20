NK
==

![](https://bitnenfer.com/libnk/libnk-splash.jpg)

**NK** is a simple **C++ 2D rendering library** with input and app control handling.

The **NK** library was designed to be an efficient and easy to use solution for creating small 2D games, like for game jams. 

The **NK** library is not a game engine. It's inteded to be used as a rendering layer for your game or engine.

**NK** is not ready for production. 

The library is formed by three C++ headers.

- `<nk/app.h>`: App code is used to control the window, input and canvas creation.

- `<nk/canvas.h>`: Canvas and Image code allows for rendering 2D images in the window created by the app.

- `<nk/hid.h>`: The HID code allows for reading mouse and keyboard input to allow for interactive applications.

You can read the **NK** API reference **[here](APIREFERENCE.md)**.

You can read on how to create a small game using **NK** and also how to setup the development environment **[here](DOCUMENTATION.md)**.

#### Supported Platforms
- Windows with DirectX 12.
- Web with WebAssembly & WebGPU.
- Web with WebAssembly & WebGL.

---
###### Developed by [Felipe Alfonso](https://bitnenfer.com/)
