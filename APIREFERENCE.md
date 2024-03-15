API Reference
================

[Back to README](README.md)

# [<nk/app.h>](https://github.com/bitnenfer/libnk/blob/main/include/nk/app.h)

### App opaque structures
```
struct NkApp;
```

### App helper macros
```
NK_COLOR_UINT(color)                // Adjust uint color to be stored in the correct order 
NK_COLOR_RGBA_UINT(r, g, b, a)      // Merge uint8_t values as an RGBA uint value.
NK_COLOR_RGB_UINT(r, g, b)          // Merge uint8_t values as an RGB uint value.
NK_COLOR_RGBA_FLOAT(r, g, b, a)     // Merge float values (0.0 to 1.0) to and RGBA uint value.
NK_COLOR_RGB_FLOAT(r, g, b)         // Merge float values (0.0 to 1.0) to and RGB uint value.
```

### App memory management function types
```
NkReallocFunc   = void* Function(void* ptr, size_t size)    // Function signature for realloc override
NkFreeFunc      = void Function(void* ptr);                 // Function signature for free override
```

### App creation structure
```
struct NkAppInfo {
    uint32_t        width;              // Window width
    uint32_t        height;             // Window height
    const char*     title;              // Window title
    uint32_t        backgroundColor;    // Background color (Used on web)
    bool            allowResize;        // Allow resize of window
    bool            fullScreen;         // Run in full screen mode (borderless)
    NkReallocFunc   reallocFunc;        // Overridable function for handling memory allocation
    NkFreeFunc      freeFunc;           // Overridable function for handling memory release
};
```

### App functions

**NkApp\* nk::app::create(const NkAppInfo& info);**

Creates a new instance of NkApp. This will create instances for NkHID and NkCanvas.

**bool nk::app::destroy(NkApp\* app);**

Destroys the instance created by `nk::app::create(...)`.

**void nk::app::update(NkApp\* app);**

Updates the internal state of app and propagates input events to NkHID.

**bool nk::app::shouldQuit(const NkApp\* app);**

Queries the app if it should exit the program.

**uint32_t nk::app::windowWidth(const NkApp\* app);**

Returns the window width.

**uint32_t nk::app::windowHeight(const NkApp\* app);**

Returns the window height.

**bool nk::app::shouldResize(const NkApp\* app, uint32_t\* newWidth, uint32_t\* newHeight);**

Returns true if the window was resized. Internally the renderer will resize the backbuffers.

**void nk::app::quit(NkApp\* app);**

Force quits the app.

**NkCanvas\* nk::app::canvas(NkApp\* app);**

Returns the canvas associated with the app.

**NkHID\* nk::app::hid(NkApp\* app);**

Returns the HID associated with the app.

# [<nk/canvas.h>](https://github.com/bitnenfer/libnk/blob/main/include/nk/canvas.h)

### Canvas opaque structures
```
struct NkCanvas;
struct NkImage;
```

### Canvas functions

**void nk::canvas::identity(NkCanvas\* canvas);**

Loads the identity matrix in the current transform matrix.

**void nk::canvas::pushMatrix(NkCanvas\* canvas);**

Pushes the current matrix into the matrix stack.

**void nk::canvas::popMatrix(NkCanvas\* canvas);**

Pops the last matrix in the matrix stack and sets it as the current transform matrix.

**void nk::canvas::translate(NkCanvas\* canvas, float x, float y);**

Applies a translation to the current transform matrix.

**void nk::canvas::rotate(NkCanvas\* canvas, float rad);**

Applies a rotation to the current transform matrix.

**void nk::canvas::scale(NkCanvas\* canvas, float x, float y);**

Applies a scale to the current transform matrix.

**void nk::canvas::drawLine(NkCanvas\* canvas, float x0, float y0, float x1, float y1, float lineWidth, uint32_t color);**

Draws a line from (x0, y0) to (x1, y1) with a line width and a line color.

**void nk::canvas::drawRect(NkCanvas\* canvas, float x, float y, float width, float height, uint32_t color);**

Draws a filled rectangle with a color.

**void nk::canvas::drawImage(NkCanvas\* canvas, float x, float y, NkImage\* image);**

Draws an image in (x, y) coordinates.

**void nk::canvas::drawImage(NkCanvas\* canvas, float x, float y, uint32_t color, NkImage\* image);**

Draws an image in (x, y) coordinates tinted by a color.

**void nk::canvas::drawImage(NkCanvas\* canvas, float x, float y, float width, float height, NkImage\* image);**

Draws an image in (x, y) coordinates with a width and height.

**void nk::canvas::drawImage(NkCanvas\* canvas, float x, float y, float width, float height, uint32_t color, NkImage\* image);**

Draws an image in (x, y) coordinates with a width, height and tinted by a color.

**void nk::canvas::drawImage(NkCanvas\* canvas, float x, float y, float frameX, float frameY, float frameWidth, float frameHeight, uint32_t color, NkImage\* image);**

Draws a frame of an image in (x, y) coordinates tinted by a color.

**void nk::canvas::drawImage(NkCanvas\* canvas, float x, float y, float width, float height, float frameX, float frameY, float frameWidth, float frameHeight, uint32_t color, NkImage\* image);**

Draws a frame of an image in (x, y) coordinates with a width, height and tinted by a color.

**void nk::canvas::beginFrame(NkCanvas\* canvas, NkImage\* renderTarget float r, float g, float b, float a);**

Setups the canvas to start drawing a frame into a render target.

**void nk::canvas::beginFrame(NkCanvas\* canvas, float r, float g, float b, float a);**

Setups the canvas to start drawing a frame.

**void nk::canvas::endFrame(NkCanvas\* canvas);**

Finishes the drawing of the frame and submits the draw commands to the GPU. 

**NkImage\* createRenderTargetImage(NkCanvas\* canvas, uint32_t width, uint32_t height);**

Creates and RGBA render target that can be used with `nk::canvas::beginFrame(...)` and `nk::canvas::drawImage(...)`. Note that you can't draw the image when it's bound as a render target.

**NkImage\* nk::canvas::createImage(NkCanvas\* canvas, uint32_t width, uint32_t height, const void\* pixels, NkImageFormat format);**

Creates an RGBA image from pixel information.

**bool nk::canvas::destroyImage(NkCanvas\* canvas, NkImage\* image);**

Destroys and image created by `nk::canvas::createImage(...)`.

### Image functions

**float nk::img::width(NkImage\* image);**

Return the width of an image.

**float nk::img::height(NkImage\* image);**

Returns the height of an image.

# [<nk/hid.h>](https://github.com/bitnenfer/libnk/blob/main/include/nk/hid.h)

### HID opaque structures
```
struct NkHID;
```

### HID enums
```
enum class NkKeyCode;
enum class NkMouseButton;
```

### HID functions

**bool nk::hid::keyDown(NkHID\* hid, NkKeyCode keyCode);**

Returns true if the keyboard key is pressed down.

**bool nk::hid::keyClick(NkHID\* hid, NkKeyCode keyCode);**

Returns true if the keyboard key is clicked.

**bool nk::hid::mouseDown(NkHID\* hid, NkMouseButton button);**

Returns true if the mouse button is pressed down.

**bool nk::hid::mouseClick(NkHID\* hid, NkMouseButton button);**

Returns true if the mouse button is clicked.

**float nk::hid::mouseWheelDeltaX(NkHID\* hid);**

Returns the deltaX of the mouse wheel.

**float nk::hid::mouseWheelDeltaY(NkHID\* hid);**

Returns the deltaY of the mouse wheel.

**void nk::hid::showCursor(NkHID\* hid, bool visible);**

Shows or hids the cursor.

**bool nk::hid::cursorVisible(NkHID\* hid);**

Returns true if the cursor is visible.

**float nk::hid::mouseX(NkHID\* hid);**

Returns the current X position of the mouse cursor.

**float nk::hid::mouseY(NkHID\* hid);**

Returns the current Y position of the mouse cursor.

---
###### Developed by [Felipe Alfonso](https://bitnenfer.com/)
