#include "wasm_canvas_webgl.h"
#include <nk/app.h>

static const char* spriteVertexShader = R"(
precision mediump float;
uniform vec2 resolution;
attribute vec2 vertPosition;
attribute vec2 vertTexCoord;
attribute vec4 vertColor;
varying vec2 fragTexCoord;
varying vec4 fragColor;
void main() {
    vec4 position = vec4((vertPosition / resolution) * 2.0 - 1.0, 0.0, 1.0);
    position.y = -position.y;
    gl_Position = position;
    fragTexCoord = vertTexCoord;
    fragColor = vertColor;
}
)";

static const char* spriteFragmentShader = R"(
precision mediump float;
uniform sampler2D mainTexture;
varying vec2 fragTexCoord;
varying vec4 fragColor;
void main() {
    gl_FragColor = texture2D(mainTexture, fragTexCoord) * fragColor;
}
)";

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
static const char* textureAtlasVertexShader = R"(
precision mediump float;
uniform vec2 resolution;
uniform vec4 rect;
attribute float vertId;
varying vec2 fragTexCoord;
void main() {
    vec4 position = vec4(0);
    vec2 texCoord = vec2(0);
    if (vertId == 0.0) {
        position = vec4(rect.xy, 0.0, 1.0);
        texCoord = vec2(0.0, 0.0);
    } else if (vertId == 1.0) {
        position = vec4(rect.x, (rect.y + rect.w), 0.0, 1.0);
        texCoord = vec2(0.0, 1.0);
    } else if (vertId == 2.0) {
        position = vec4((rect.x + rect.z), (rect.y + rect.w), 0.0, 1.0);
        texCoord = vec2(1.0, 1.0);
    } else if (vertId == 3.0) {
        position = vec4((rect.x + rect.z), rect.y, 0.0, 1.0);
        texCoord = vec2(1.0, 0.0);
    }
    position.xy = (position.xy / resolution) * 2.0 - 1.0;
    gl_Position = position;
    fragTexCoord = texCoord;
}
)";

static const char* textureAtlasFragmentShader = R"(
precision mediump float;
uniform sampler2D texture;
varying vec2 fragTexCoord;
void main() {
    gl_FragColor = texture2D(texture, fragTexCoord);
}

)";
#endif

static NkWebGLInstance webGLinstance{};

static GLuint compileGLShader(GLenum type, const char* shaderCode) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char buffer[256] = {};
        GLsizei length = 0;
        glGetShaderInfoLog(shader, 256, &length, buffer);
        printf("Faile to compile %s shader:\n%s",
               type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", buffer);
        return 0;
    }
    return shader;
}

static GLuint compileGLProgram(GLuint vertShader, GLuint fragShader) {

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char buffer[256] = {};
        GLsizei length = 0;
        glGetProgramInfoLog(program, 256, &length, buffer);
        printf("Failed to link program.\n%s", buffer);
        return 0;
    }
    return program;
}

NkWebGLInstance* nk::webgl::createInstance() {
    if (webGLinstance.refCount == 0) {
        EmscriptenWebGLContextAttributes contextAttribs{};
        contextAttribs.alpha = false;
        contextAttribs.depth = false;
        contextAttribs.stencil = false;
        contextAttribs.antialias = false;
        contextAttribs.premultipliedAlpha = false;
        contextAttribs.preserveDrawingBuffer = false;
        contextAttribs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;
        contextAttribs.failIfMajorPerformanceCaveat = false;
        contextAttribs.majorVersion = 1;
        contextAttribs.minorVersion = 0;
        contextAttribs.enableExtensionsByDefault = true;
        contextAttribs.explicitSwapControl = false;
        contextAttribs.proxyContextToMainThread =
            EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
        contextAttribs.renderViaOffscreenBackBuffer = false;
        webGLinstance.context =
            emscripten_webgl_create_context("#nk-canvas", &contextAttribs);
        emscripten_webgl_make_context_current(webGLinstance.context);
    }
    webGLinstance.refCount++;
    return &webGLinstance;
}
void nk::webgl::destroyInstance(NkWebGLInstance* instance) {
    if (--webGLinstance.refCount == 0) {
        // destroy..
    }
}
NkWebGLInstance* nk::webgl::instance() { return &webGLinstance; }

NkCanvas* nk::canvas::create(NkApp* app, bool allowResize) {
    nk::webgl::createInstance();
    NkCanvas* canvas = (NkCanvas*)nk::utils::memZeroAlloc(1, sizeof(NkCanvas));
    if (!canvas) {
        return nullptr;
    }
    canvas->base.init(canvas, (float)app->windowWidth,
                      (float)app->windowHeight);
    canvas->app = app;
    canvas->allowResize = allowResize;
    canvas->clearColor[0] = 0.0f;
    canvas->clearColor[1] = 0.0f;
    canvas->clearColor[2] = 0.0f;
    canvas->clearColor[3] = 1.0f;
    canvas->spriteVertShader =
        compileGLShader(GL_VERTEX_SHADER, spriteVertexShader);
    canvas->spriteFragShader =
        compileGLShader(GL_FRAGMENT_SHADER, spriteFragmentShader);
    canvas->spriteProgram =
        compileGLProgram(canvas->spriteVertShader, canvas->spriteFragShader);
    GLuint indexBuffer = (GLuint)canvas->base.gpuIndexBuffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, NK_CANVAS_MAX_INDICES_BYTE_SIZE,
                    canvas->base.indexBufferData());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    canvas->textureAtlasVertShader =
        compileGLShader(GL_VERTEX_SHADER, textureAtlasVertexShader);
    canvas->textureAtlasFragShader =
        compileGLShader(GL_FRAGMENT_SHADER, textureAtlasFragmentShader);
    canvas->textureAtlasProgram = compileGLProgram(
        canvas->textureAtlasVertShader, canvas->textureAtlasFragShader);
    glGenBuffers(1, &canvas->textureAtlasVB);
    float textureAtlasVerts[] = {0.0f, 1.0f, 2.0f, 0.0f, 2.0f, 3.0f};
    glBindBuffer(GL_ARRAY_BUFFER, canvas->textureAtlasVB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureAtlasVerts), textureAtlasVerts,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif

    return canvas;
}
bool nk::canvas::destroy(NkCanvas* canvas) {
    if (canvas) {
        canvas->base.destroy(canvas);
        nk::utils::memFree(canvas);
    }
    return false;
}
void nk::canvas::identity(NkCanvas* canvas) { canvas->base.loadIdentity(); }

void nk::canvas::pushMatrix(NkCanvas* canvas) { canvas->base.pushMatrix(); }

void nk::canvas::popMatrix(NkCanvas* canvas) { canvas->base.popMatrix(); }

void nk::canvas::translate(NkCanvas* canvas, float x, float y) {
    canvas->base.translate(x, y);
}

void nk::canvas::rotate(NkCanvas* canvas, float rad) {
    canvas->base.rotate(rad);
}

void nk::canvas::scale(NkCanvas* canvas, float x, float y) {
    canvas->base.scale(x, y);
}

void nk::canvas::drawLine(NkCanvas* canvas, float x0, float y0, float x1,
                          float y1, float lineWidth, uint32_t color) {
    canvas->base.drawLine(x0, y0, x1, y1, lineWidth, color);
}

void nk::canvas::drawRect(NkCanvas* canvas, float x, float y, float width,
                          float height, uint32_t color) {
    canvas->base.drawRect(x, y, width, height, color);
}

#define FIXED_DRAW_IMAGE(x, y, ...)                                            \
    if (!(image && image->framebuffer)) {                                      \
        canvas->base.drawImage(x, y, __VA_ARGS__);                             \
    } else {                                                                   \
        canvas->base.pushMatrix();                                             \
        canvas->base.translate(x, y + image->height);                          \
        canvas->base.scale(1.0f, -1.0f);                                       \
        canvas->base.drawImage(0, 0, __VA_ARGS__);                             \
        canvas->base.popMatrix();                                              \
    }

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, NkImage* image) {
    FIXED_DRAW_IMAGE(x, y, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, uint32_t color,
                           NkImage* image) {
    FIXED_DRAW_IMAGE(x, y, color, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, NkImage* image) {

    FIXED_DRAW_IMAGE(x, y, width, height, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, uint32_t color, NkImage* image) {
    FIXED_DRAW_IMAGE(x, y, width, height, color, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float frameX,
                           float frameY, float frameWidth, float frameHeight,
                           uint32_t color, NkImage* image) {
    FIXED_DRAW_IMAGE(x, y, frameX, frameY, frameWidth, frameHeight, color,
                     image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, float frameX, float frameY,
                           float frameWidth, float frameHeight, uint32_t color,
                           NkImage* image) {
    FIXED_DRAW_IMAGE(x, y, width, height, frameX, frameY, frameWidth,
                     frameHeight, color, image);
}

#undef FIXED_DRAW_IMAGE

void nk::canvas::beginFrame(NkCanvas* canvas, float r, float g, float b,
                            float a) {
    canvas->clearColor[0] = r;
    canvas->clearColor[1] = g;
    canvas->clearColor[2] = b;
    canvas->clearColor[3] = a;
    canvas->base.beginFrame(canvas);
}

void nk::canvas::beginFrame(NkCanvas* canvas, NkImage* renderTarget, float r,
                            float g, float b, float a) {
    canvas->clearColor[0] = r;
    canvas->clearColor[1] = g;
    canvas->clearColor[2] = b;
    canvas->clearColor[3] = a;
    canvas->renderTarget = renderTarget;
    canvas->base.beginFrame(canvas);
}

void nk::canvas::endFrame(NkCanvas* canvas) {
    canvas->base.endFrame(canvas);
    nk::webgl::drawFrame(canvas, canvas->base.currentFrameIndex);
    canvas->base.swapFrame(canvas);
    canvas->renderTarget = nullptr;
}

void nk::canvas::present(NkCanvas* canvas) {}

float nk::canvas::viewWidth(NkCanvas* canvas) {
    return canvas->base.resolution[0];
}

float nk::canvas::viewHeight(NkCanvas* canvas) {
    return canvas->base.resolution[1];
}
NkImage* nk::canvas::createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                             uint32_t height) {
    NkImage* image = createImage(canvas, width, height, nullptr);

    glGenFramebuffers(1, &image->framebuffer);
    glGenRenderbuffers(1, &image->renderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, image->framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, image->renderbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           image->texture, 0);
    NK_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
                  GL_FRAMEBUFFER_COMPLETE,
              "Framebuffer not complete");
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return image;
}
NkImage* nk::canvas::createImage(NkCanvas* canvas, uint32_t width,
                                 uint32_t height, const void* pixels,
                                 NkImageFormat format) {
    NkImage* image = (NkImage*)nk::utils::memZeroAlloc(1, sizeof(NkImage));
    if (!image) {
        return nullptr;
    }

    glGenTextures(1, &image->texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);

    image->width = (float)width;
    image->height = (float)height;

    return image;
}
bool nk::canvas::destroyImage(NkCanvas* canvas, NkImage* image) {
    if (image) {
        glDeleteTextures(1, &image->texture);
        if (image->framebuffer) {
            glDeleteFramebuffers(1, &image->framebuffer);
            glDeleteRenderbuffers(1, &image->renderbuffer);
        }
        return true;
    }
    return false;
}

float nk::img::width(NkImage* image) { return image->width; }
float nk::img::height(NkImage* image) { return image->height; }

void nk::canvas_internal::initVertexBuffer(NkCanvasVertexBuffer* vertexBuffer,
                                           size_t bufferSize) {
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    vertexBuffer->gpuVertexBuffer = (void*)buffer;
}
void nk::canvas_internal::initIndexBuffer(void** gpuIndexBuffer) {
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, NK_CANVAS_MAX_INDICES_BYTE_SIZE,
                 nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    *gpuIndexBuffer = (void*)buffer;
}
void nk::canvas_internal::destroyVertexBuffer(
    NkCanvasVertexBuffer* vertexBuffer) {
    GLuint buffer = (GLuint)vertexBuffer->gpuVertexBuffer;
    glDeleteBuffers(1, &buffer);
    vertexBuffer->gpuVertexBuffer = nullptr;
}
void nk::canvas_internal::destroyIndexBuffer(void** gpuIndexBuffer) {
    GLuint buffer = (GLuint)*gpuIndexBuffer;
    glDeleteBuffers(1, &buffer);
    *gpuIndexBuffer = nullptr;
}
void nk::canvas_internal::signalFrameSyncPoint(NkCanvas* canvas,
                                               NkGPUHandle gpuSyncPoint,
                                               uint64_t value) {}
void nk::canvas_internal::waitFrameSyncPoint(NkGPUHandle gpuSyncPoint,
                                             uint64_t value) {}
void nk::canvas_internal::initFrameSyncPoint(NkGPUHandle* gpuSyncPoint) {}
void nk::canvas_internal::destroyFrameSyncPoint(NkGPUHandle* gpuSyncPoint) {}
NkCanvasBase* nk::canvas_internal::canvasBase(NkCanvas* canvas) {
    return &canvas->base;
}

void nk::webgl::drawFrame(NkCanvas* canvas, uint64_t currentFrameIndex) {
    float viewWidth = nk::canvas::viewWidth(canvas);
    float viewHeight = nk::canvas::viewHeight(canvas);
    if (canvas->renderTarget) {
        viewWidth = canvas->renderTarget->width;
        viewHeight = canvas->renderTarget->height;
    }

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    nk::webgl::updateTextureAtlas(canvas, canvas->base.frameTextureAtlas);
#endif

    GLuint mainTextureLocation =
        glGetUniformLocation(canvas->spriteProgram, "mainTexture");
    GLuint resolutionLocation =
        glGetUniformLocation(canvas->spriteProgram, "resolution");
    GLuint vertPositionLocation =
        glGetAttribLocation(canvas->spriteProgram, "vertPosition");
    GLuint vertTexCoordLocation =
        glGetAttribLocation(canvas->spriteProgram, "vertTexCoord");
    GLuint vertColorLocation =
        glGetAttribLocation(canvas->spriteProgram, "vertColor");

    glUseProgram(canvas->spriteProgram);
    glViewport(0, 0, (GLsizei)viewWidth, (GLsizei)viewHeight);
    glScissor(0, 0, (GLsizei)viewWidth, (GLsizei)viewHeight);
    glDepthRangef(0.0f, 1.0f);
    glUniform2f(resolutionLocation, viewWidth, viewHeight);
    glUniform1i(mainTextureLocation, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas->renderTarget
                                          ? canvas->renderTarget->framebuffer
                                          : 0);
    glClearColor(canvas->clearColor[0], canvas->clearColor[1],
                 canvas->clearColor[2], canvas->clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    GLuint indexBuffer = (GLuint)canvas->base.gpuIndexBuffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        ((NkImage*)canvas->base.frameTextureAtlas.gpuTexture)->texture);
#endif

    GLuint indexOffset = 0;
    NkCanvasDrawBatchInternalArray& drawBatchArray =
        canvas->base.drawBatchArray[currentFrameIndex];
    for (uint32_t index = 0; index < drawBatchArray.drawBatchNum; ++index) {
        NkCanvasDrawBatchInternal& drawBatch =
            drawBatchArray.drawBatches[index];
        NkCanvasVertexBuffer* vertexBuffer = drawBatch.buffer;
        GLuint buffer = (GLuint)vertexBuffer->gpuVertexBuffer;
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        vertexBuffer->vertexCount * sizeof(NkCanvasVertex),
                        vertexBuffer->vertices);
        glEnableVertexAttribArray(vertPositionLocation);
        glEnableVertexAttribArray(vertTexCoordLocation);
        glEnableVertexAttribArray(vertColorLocation);
        glVertexAttribPointer(vertPositionLocation, 2, GL_FLOAT, false,
                              sizeof(NkCanvasVertex),
                              (void*)offsetof(NkCanvasVertex, position));
        glVertexAttribPointer(vertTexCoordLocation, 2, GL_FLOAT, false,
                              sizeof(NkCanvasVertex),
                              (void*)offsetof(NkCanvasVertex, texCoord));
        glVertexAttribPointer(vertColorLocation, 4, GL_UNSIGNED_BYTE, true,
                              sizeof(NkCanvasVertex),
                              (void*)offsetof(NkCanvasVertex, color));
#if !NK_CANVAS_TEXTURE_ATLAS_ENABLED
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, drawBatch.image->texture);
#endif
        glDrawElements(GL_TRIANGLES, drawBatch.count, GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(indexOffset * sizeof(GLuint)));

        indexOffset += drawBatch.count;
    }
}

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
void nk::canvas_internal::initTextureAtlasResource(
    NkCanvas* canvas, NkTextureAtlas& textureAtlas) {
    textureAtlas.gpuTexture = (void*)nk::canvas::createRenderTargetImage(
        canvas, textureAtlas.width, textureAtlas.height);
}
void nk::canvas_internal::destroyTextureAtlasResource(
    NkTextureAtlas& textureAtlas) {
    NkImage* image = (NkImage*)textureAtlas.gpuTexture;
    glDeleteTextures(1, &image->texture);
    glDeleteFramebuffers(1, &image->framebuffer);
    glDeleteRenderbuffers(1, &image->renderbuffer);
    textureAtlas.gpuTexture = nullptr;
}
void nk::canvas_internal::setTextureAtlasState(NkImage* image,
                                               const NkTextureAtlasRect& rect) {
    image->state |= NK_IMAGE_BIT_TEXTURE_ATLAS;
    image->rect = rect;
}

bool nk::canvas_internal::isImageInTextureAtlas(NkImage* image) {
    return (image->state & NK_IMAGE_BIT_TEXTURE_ATLAS) > 0;
}

const NkTextureAtlasRect& nk::canvas_internal::textureRect(NkImage* image) {
    return image->rect;
}

void nk::webgl::updateTextureAtlas(NkCanvas* canvas,
                                   NkTextureAtlas& textureAtlas) {
    NkImage* textureAtlasImage = (NkImage*)textureAtlas.gpuTexture;
    glViewport(0, 0, (GLsizei)textureAtlas.width, (GLsizei)textureAtlas.height);
    glScissor(0, 0, (GLsizei)textureAtlas.width, (GLsizei)textureAtlas.height);
    glBindFramebuffer(GL_FRAMEBUFFER, textureAtlasImage->framebuffer);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(canvas->textureAtlasProgram);
    glBindBuffer(GL_ARRAY_BUFFER, canvas->textureAtlasVB);
    GLuint resolutionLocation =
        glGetUniformLocation(canvas->textureAtlasProgram, "resolution");
    GLuint rectLocation =
        glGetUniformLocation(canvas->textureAtlasProgram, "rect");
    GLuint vertIdLocation =
        glGetAttribLocation(canvas->textureAtlasProgram, "vertId");

    glUniform2f(resolutionLocation, (float)textureAtlas.width,
                (float)textureAtlas.height);
    glEnableVertexAttribArray(vertIdLocation);
    glVertexAttribPointer(vertIdLocation, 1, GL_FLOAT, false, sizeof(float),
                          nullptr);
    for (auto& entry : textureAtlas.images) {
        NkImage* image = entry.first;
        NkTextureAtlasRect& rect = entry.second;
        glUniform4f(rectLocation, rect.x, rect.y, rect.width, rect.height);
        glBindTexture(GL_TEXTURE_2D, image->texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        image->state &= ~NK_IMAGE_BIT_TEXTURE_ATLAS;
    }
}

#endif