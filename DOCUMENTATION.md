Documentation
================

[Back to README](README.md)

### Minimal Example
Here you can find a small example of how to setup and start drawing with **NK**.

This example will render a yellow rectangle that rotates and moves with the mouse cursor.

```
#include <nk/app.h>
#include <nk/canvas.h>
#include <nk/hid.h>

int main() {
    NkApp* app = nk::app::create({ 960, 640, "Demo" });
    NkCanvas* canvas = nk::app::canvas(app);
    NkHID* hid = nk::app::hid(app);
    float rotation = 0.0f;

    while (!nk::app::shouldQuit(app)) {
        nk::app::update(app);

        if (nk::hid::keyClick(hid, NkKeyCode::ESC)) {
            nk::app::quit(app);
        }

        nk::canvas::beginFrame(canvas);
        nk::canvas::pushMatrix(canvas);
        nk::canvas::translate(canvas, nk::hid::mouseX(hid), nk::hid::mouseY(hid));
        nk::canvas::rotate(canvas, rotation);
        nk::canvas::drawRect(canvas, -50.0f, -50.0f, 100.0f, 100.0f, NK_COLOR_RGB_UINT(0xff, 0xff, 0x00));
        nk::canvas::popMatrix(canvas);
        nk::canvas::endFrame(canvas);
        
        rotation += 1.0f / 60.0f;
    }

    nk::app::destroy(app);
    return 0;
}

```

You can test this two examples running in **WebGL**:
- [NIGO (Small game)](https://bitnenfer.com/libnk/webgl/nigo/) - Source code [here](https://github.com/bitnenfer/libnk/tree/main/examples/nigogame).

- [Draw image](https://bitnenfer.com/libnk/webgl/draw/) - Source code [here](https://github.com/bitnenfer/nk/tree/main/examples/draw_image).


If your browser supports **WebGPU** you can test this two examples:

- [NIGO (Small game)](https://bitnenfer.com/libnk/nigo/) - Source code [here](https://github.com/bitnenfer/libnk/tree/main/examples/nigogame).

- [Draw multiple images](https://bitnenfer.com/libnk/draw/) - Source code [here](https://github.com/bitnenfer/nk/tree/main/examples/draw_multiple_images).


You can also find more examples [here](https://github.com/bitnenfer/libnk/tree/main/examples).

### Using NK

#### Windows
First you need to have [Visual Studio](https://visualstudio.microsoft.com/) installed with the C++ toolchain. Also you can add CMake to allow generating the required Visual Studio files for compiling.

You can open the **NK** directory with Visual Studio or VSCode (if you have msvc toolchain). If you are going to use VSCode I recommend installing Microsoft's CMake Tools extension.

You can use the [examples](https://github.com/bitnenfer/libnk/tree/main/examples) as a starting point.

#### Web (WebAssembly)
For compiling to WebAssembly (WASM) you'll need to install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) first. Then you'll need to create a `emsdk_path.txt` at the root of the **NK** folder and write the path of where the EMSDK is installed. With that you'll be able to compile the library and projects that use it. 

---
###### Developed by [Felipe Alfonso](https://bitnenfer.com/)
