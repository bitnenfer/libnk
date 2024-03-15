#include "wasm_canvas_webgpu.h"

const char shaderSource[] = R"(

struct VertexIn {
    @location(0) position : vec2<f32>,
    @location(1) texCoord : vec2<f32>,
    @location(2) color : vec4<f32>
};

struct VertexOut {
    @builtin(position) position : vec4<f32>,
    @location(1) texCoord : vec2<f32>,
    @location(2) color : vec4<f32>
};

@group(0) @binding(0) var<uniform> resolution : vec2<f32>;

@vertex
fn vsMain(vtx : VertexIn) -> VertexOut {
    var vtxOut : VertexOut;
    vtxOut.position = vec4<f32>((vtx.position / resolution) * 2.0 - 1.0, 0.0, 1.0);
	vtxOut.position.y = -vtxOut.position.y;
    vtxOut.texCoord = vtx.texCoord;
    vtxOut.color = vtx.color;
    return vtxOut;
}

@group(0) @binding(1) var samplerPoint : sampler;
@group(1) @binding(0) var mainTexture : texture_2d<f32>;
@fragment
fn fsMain(vtx : VertexOut) -> @location(0) vec4<f32> {
    return textureSample(mainTexture, samplerPoint, vtx.texCoord) * vtx.color;
}

)";

static NkWebGPUInstance webGPUInstance = {};

NkWebGPUInstance* nk::webgpu::createInstance() {
    if (!webGPUInstance.device) {
        WGPUInstanceDescriptor instanceDescriptor{};
        instanceDescriptor.nextInChain = nullptr;

        webGPUInstance.instance = wgpuCreateInstance(&instanceDescriptor);
        webGPUInstance.adapter = (WGPUAdapter)1;
        webGPUInstance.device = (WGPUDevice)1;
        webGPUInstance.queue = wgpuDeviceGetQueue(webGPUInstance.device);
    }
    webGPUInstance.refCount++;
    return &webGPUInstance;
}

void nk::webgpu::destroyInstance(NkWebGPUInstance* instance) {
    if (--webGPUInstance.refCount == 0) {
        // destroy.
    }
}

NkWebGPUInstance* nk::webgpu::instance() { return &webGPUInstance; }

void nk::canvas_internal::initVertexBuffer(NkCanvasVertexBuffer* vertexBuffer,
                                           size_t bufferSize) {
    NkWebGPUBuffer* buffer =
        (NkWebGPUBuffer*)nk::utils::memZeroAlloc(1, sizeof(NkWebGPUBuffer));
    if (buffer) {
        WGPUBufferDescriptor bufferDesc{};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = "NkVertexBuffer::buffer";
        bufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        bufferDesc.size = bufferSize;
        bufferDesc.mappedAtCreation = false;
        buffer->buffer =
            wgpuDeviceCreateBuffer(nk::webgpu::instance()->device, &bufferDesc);
        vertexBuffer->gpuVertexBuffer = (void*)buffer;
    }
}

void nk::canvas_internal::initIndexBuffer(void** gpuIndexBuffer) {
    NkWebGPUBuffer* buffer =
        (NkWebGPUBuffer*)nk::utils::memZeroAlloc(1, sizeof(NkWebGPUBuffer));
    if (buffer) {
        WGPUBufferDescriptor bufferDesc{};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = "NkIndexBuffer::buffer";
        bufferDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
        bufferDesc.size = NK_CANVAS_MAX_INDICES_BYTE_SIZE;
        bufferDesc.mappedAtCreation = true;
        buffer->buffer =
            wgpuDeviceCreateBuffer(nk::webgpu::instance()->device, &bufferDesc);
        *gpuIndexBuffer = (void*)buffer;
    }
}

void nk::canvas_internal::destroyVertexBuffer(
    NkCanvasVertexBuffer* vertexBuffer) {
    if (vertexBuffer && vertexBuffer->gpuVertexBuffer) {
        NkWebGPUBuffer* buffer = (NkWebGPUBuffer*)vertexBuffer->gpuVertexBuffer;
        wgpuBufferDestroy(buffer->buffer);
        nk::utils::memFree(buffer);
    }
}

void nk::canvas_internal::destroyIndexBuffer(void** gpuIndexBuffer) {
    if (gpuIndexBuffer && *gpuIndexBuffer) {
        NkWebGPUBuffer* buffer = (NkWebGPUBuffer*)*gpuIndexBuffer;
        wgpuBufferDestroy(buffer->buffer);
        nk::utils::memFree(buffer);
    }
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

NkCanvas* nk::canvas::create(NkApp* app, bool allowResize) {
    nk::webgpu::createInstance();
    NkCanvas* canvas = (NkCanvas*)nk::utils::memZeroAlloc(1, sizeof(NkCanvas));
    if (!canvas)
        return nullptr;
    canvas->base.init(canvas, (float)app->windowWidth,
                      (float)app->windowHeight);
    canvas->app = app;
    canvas->allowResize = allowResize;
    canvas->clearColor[0] = 0.0f;
    canvas->clearColor[1] = 0.0f;
    canvas->clearColor[2] = 0.0f;
    canvas->clearColor[3] = 1.0f;

    WGPUSurfaceDescriptorFromCanvasHTMLSelector surfaceFromCanvas{};
    surfaceFromCanvas.chain.next = nullptr;
    surfaceFromCanvas.chain.sType =
        WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    surfaceFromCanvas.selector = "#nk-canvas";

    WGPUSurfaceDescriptor surfaceDesc{};
    surfaceDesc.nextInChain = &surfaceFromCanvas.chain;
    surfaceDesc.label = "CanvasSurface";
    canvas->canvasSurface = wgpuInstanceCreateSurface(
        nk::webgpu::instance()->instance, &surfaceDesc);
    // canvas->surfaceFormat = wgpuSurfaceGetPreferredFormat(
    //     canvas->canvasSurface, nk::webgpu::instance()->adapter);
    canvas->surfaceFormat = WGPUTextureFormat_RGBA8Unorm;
    WGPUSwapChainDescriptor swapChainDesc{};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.label = "CanvasSwapChain";
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.format = canvas->surfaceFormat;
    swapChainDesc.width = app->windowWidth;   // info.width;
    swapChainDesc.height = app->windowHeight; // info.height;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    canvas->swapChain = wgpuDeviceCreateSwapChain(
        nk::webgpu::instance()->device, canvas->canvasSurface, &swapChainDesc);

    WGPUShaderModuleWGSLDescriptor wgslModuleDesc{};
    wgslModuleDesc.chain.next = nullptr;
    wgslModuleDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgslModuleDesc.code = shaderSource;
    WGPUShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &wgslModuleDesc.chain;
    shaderDesc.label = "SpriteShader";
    canvas->spriteShaderModule = wgpuDeviceCreateShaderModule(
        nk::webgpu::instance()->device, &shaderDesc);

    WGPUBindGroupLayoutEntry bindGroup0LayoutEntriesGroup[2] = {};
    // resolution binding
    bindGroup0LayoutEntriesGroup[0].nextInChain = nullptr;
    bindGroup0LayoutEntriesGroup[0].binding = 0;
    bindGroup0LayoutEntriesGroup[0].visibility = WGPUShaderStage_Vertex;
    bindGroup0LayoutEntriesGroup[0].buffer.nextInChain = nullptr;
    bindGroup0LayoutEntriesGroup[0].buffer.type = WGPUBufferBindingType_Uniform;
    bindGroup0LayoutEntriesGroup[0].buffer.hasDynamicOffset = false;
    bindGroup0LayoutEntriesGroup[0].buffer.minBindingSize = 0;

    // sampler binding
    bindGroup0LayoutEntriesGroup[1].nextInChain = nullptr;
    bindGroup0LayoutEntriesGroup[1].binding = 1;
    bindGroup0LayoutEntriesGroup[1].visibility = WGPUShaderStage_Fragment;
    bindGroup0LayoutEntriesGroup[1].sampler.nextInChain = nullptr;
    bindGroup0LayoutEntriesGroup[1].sampler.type =
        WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor bindGroup0LayoutDesc{};
    bindGroup0LayoutDesc.nextInChain = nullptr;
    bindGroup0LayoutDesc.label = "SpriteBindGroupLayout_0";
    bindGroup0LayoutDesc.entryCount = 2;
    bindGroup0LayoutDesc.entries = bindGroup0LayoutEntriesGroup;
    canvas->spriteBindGroupLayout[0] = wgpuDeviceCreateBindGroupLayout(
        nk::webgpu::instance()->device, &bindGroup0LayoutDesc);

    WGPUBindGroupLayoutEntry bindGroup1LayoutEntriesGroup[1] = {};
    bindGroup1LayoutEntriesGroup[0].nextInChain = nullptr;
    bindGroup1LayoutEntriesGroup[0].binding = 0;
    bindGroup1LayoutEntriesGroup[0].visibility = WGPUShaderStage_Fragment;
    bindGroup1LayoutEntriesGroup[0].texture.nextInChain = nullptr;
    bindGroup1LayoutEntriesGroup[0].texture.sampleType =
        WGPUTextureSampleType_Float;
    bindGroup1LayoutEntriesGroup[0].texture.viewDimension =
        WGPUTextureViewDimension_2D;
    bindGroup1LayoutEntriesGroup[0].texture.multisampled = false;

    WGPUBindGroupLayoutDescriptor bindGroup1LayoutDesc{};
    bindGroup1LayoutDesc.nextInChain = nullptr;
    bindGroup1LayoutDesc.label = "SpriteBindGroupLayout_1";
    bindGroup1LayoutDesc.entryCount = 1;
    bindGroup1LayoutDesc.entries = bindGroup1LayoutEntriesGroup;
    canvas->spriteBindGroupLayout[1] = wgpuDeviceCreateBindGroupLayout(
        nk::webgpu::instance()->device, &bindGroup1LayoutDesc);

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.nextInChain = nullptr;
    pipelineLayoutDesc.label = "SpritePipelineLayout";
    pipelineLayoutDesc.bindGroupLayoutCount = 2;
    pipelineLayoutDesc.bindGroupLayouts = canvas->spriteBindGroupLayout;
    canvas->spritePipelineLayout = wgpuDeviceCreatePipelineLayout(
        nk::webgpu::instance()->device, &pipelineLayoutDesc);

    WGPUVertexAttribute vertexAttribs[3] = {};
    vertexAttribs[0].format = WGPUVertexFormat_Float32x2;
    vertexAttribs[0].offset = offsetof(NkCanvasVertex, position);
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[1].format = WGPUVertexFormat_Float32x2;
    vertexAttribs[1].offset = offsetof(NkCanvasVertex, texCoord);
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[2].format = WGPUVertexFormat_Unorm8x4;
    vertexAttribs[2].offset = offsetof(NkCanvasVertex, color);
    vertexAttribs[2].shaderLocation = 2;

    WGPUVertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.arrayStride = sizeof(NkCanvasVertex);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout.attributeCount = 3;
    vertexBufferLayout.attributes = vertexAttribs;

    WGPUBlendState blendState{};
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor = WGPUBlendFactor_One;
    blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.nextInChain = nullptr;
    colorTargetState.format = canvas->surfaceFormat;
    colorTargetState.blend = &blendState; // TODO: needs to be filled.
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.nextInChain = nullptr;
    fragState.module = canvas->spriteShaderModule;
    fragState.entryPoint = "fsMain";
    fragState.constantCount = 0;
    fragState.constants = nullptr;
    fragState.targetCount = 1;
    fragState.targets = &colorTargetState;

    WGPURenderPipelineDescriptor renderPipelineDesc{};
    renderPipelineDesc.nextInChain = nullptr;
    renderPipelineDesc.label = "SpritePSO";
    renderPipelineDesc.layout = canvas->spritePipelineLayout;
    renderPipelineDesc.vertex.nextInChain = nullptr;
    renderPipelineDesc.vertex.module = canvas->spriteShaderModule;
    renderPipelineDesc.vertex.entryPoint = "vsMain";
    renderPipelineDesc.vertex.constantCount = 0;
    renderPipelineDesc.vertex.constants = nullptr;
    renderPipelineDesc.vertex.bufferCount = 1;
    renderPipelineDesc.vertex.buffers = &vertexBufferLayout;
    renderPipelineDesc.primitive.nextInChain = nullptr;
    renderPipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    renderPipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    renderPipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
    renderPipelineDesc.primitive.cullMode = WGPUCullMode_None;
    renderPipelineDesc.depthStencil = nullptr;
    renderPipelineDesc.multisample.nextInChain = nullptr;
    renderPipelineDesc.multisample.alphaToCoverageEnabled = false;
    renderPipelineDesc.multisample.count = 1;
    renderPipelineDesc.multisample.mask = 0xffffffff;
    renderPipelineDesc.fragment = &fragState;
    canvas->spritePSO = wgpuDeviceCreateRenderPipeline(
        nk::webgpu::instance()->device, &renderPipelineDesc);

    // upload index buffer
    NkWebGPUBuffer* indexBuffer = (NkWebGPUBuffer*)canvas->base.gpuIndexBuffer;
    void* mappedIndexBuffer = wgpuBufferGetMappedRange(
        indexBuffer->buffer, 0, NK_CANVAS_MAX_INDICES_BYTE_SIZE);
    memcpy(mappedIndexBuffer, canvas->base.indexBufferData(),
           NK_CANVAS_MAX_INDICES_BYTE_SIZE);
    wgpuBufferUnmap(indexBuffer->buffer);

    // resolution buffer
    WGPUBufferDescriptor bufferDesc{};
    bufferDesc.nextInChain = nullptr;
    bufferDesc.label = "Canvas::resolutionBuffer";
    bufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDesc.size = sizeof(float) * 2;
    bufferDesc.mappedAtCreation = false;
    canvas->resolutionBuffer =
        wgpuDeviceCreateBuffer(nk::webgpu::instance()->device, &bufferDesc);

    WGPUSamplerDescriptor samplerDesc{};
    samplerDesc.nextInChain = nullptr;
    samplerDesc.label = "PointSampler";
    samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = WGPUFilterMode_Nearest;
    samplerDesc.minFilter = WGPUFilterMode_Nearest;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 32.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    canvas->pointSampler =
        wgpuDeviceCreateSampler(nk::webgpu::instance()->device, &samplerDesc);

    WGPUBindGroupEntry staticBindGroupEntries[2] = {};
    staticBindGroupEntries[0].nextInChain = nullptr;
    staticBindGroupEntries[0].binding = 0;
    staticBindGroupEntries[0].buffer = canvas->resolutionBuffer;
    staticBindGroupEntries[0].offset = 0;
    staticBindGroupEntries[0].size = sizeof(float) * 2;
    staticBindGroupEntries[0].sampler = nullptr;
    staticBindGroupEntries[0].textureView = nullptr;

    staticBindGroupEntries[1].nextInChain = nullptr;
    staticBindGroupEntries[1].binding = 1;
    staticBindGroupEntries[1].buffer = nullptr;
    staticBindGroupEntries[1].offset = 0;
    staticBindGroupEntries[1].size = 0;
    staticBindGroupEntries[1].sampler = canvas->pointSampler;
    staticBindGroupEntries[1].textureView = nullptr;

    WGPUBindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.label = "StaticBindGroup";
    bindGroupDesc.layout = canvas->spriteBindGroupLayout[0];
    bindGroupDesc.entryCount = 2;
    bindGroupDesc.entries = staticBindGroupEntries;
    canvas->staticBindGroup = wgpuDeviceCreateBindGroup(
        nk::webgpu::instance()->device, &bindGroupDesc);
    canvas->renderTarget = nullptr;
    return canvas;
}

bool nk::canvas::destroy(NkCanvas* canvas) {
    if (canvas) {
        canvas->base.destroy(canvas);
        nk::utils::memFree(canvas);
        nk::webgpu::destroyInstance(&webGPUInstance);
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

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, NkImage* image) {
    canvas->base.drawImage(x, y, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, uint32_t color,
                           NkImage* image) {
    canvas->base.drawImage(x, y, color, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, NkImage* image) {
    canvas->base.drawImage(x, y, width, height, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, uint32_t color, NkImage* image) {
    canvas->base.drawImage(x, y, width, height, color, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float frameX,
                           float frameY, float frameWidth, float frameHeight,
                           uint32_t color, NkImage* image) {
    canvas->base.drawImage(x, y, frameX, frameY, frameWidth, frameHeight, color,
                           image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, float frameX, float frameY,
                           float frameWidth, float frameHeight, uint32_t color,
                           NkImage* image) {
    canvas->base.drawImage(x, y, width, height, frameX, frameY, frameWidth,
                           frameHeight, color, image);
}

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
    nk::webgpu::drawFrame(canvas, canvas->base.currentFrameIndex);
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

NkImage* nk::canvas::createImage(NkCanvas* canvas, uint32_t width,
                                 uint32_t height, const void* pixels,
                                 NkImageFormat format) {
    NkImage* image = (NkImage*)nk::utils::memZeroAlloc(1, sizeof(NkImage));
    if (!image)
        return nullptr;

    size_t pixelSize = sizeof(uint32_t);
    size_t imageDataSize = pixelSize * width * height;

    WGPUTextureDescriptor textureDesc{};
    textureDesc.nextInChain = nullptr;
    textureDesc.label = "NkImage::texture";
    textureDesc.usage = WGPUTextureUsage_TextureBinding |
                        WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = {width, height, 1};
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    image->texture =
        wgpuDeviceCreateTexture(nk::webgpu::instance()->device, &textureDesc);

    WGPUImageCopyTexture copyTextureDesc{};
    copyTextureDesc.nextInChain = nullptr;
    copyTextureDesc.texture = image->texture;
    copyTextureDesc.mipLevel = 0;
    copyTextureDesc.origin = {0, 0, 0};
    copyTextureDesc.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout dataLayout{};
    dataLayout.nextInChain = nullptr;
    dataLayout.offset = 0;
    dataLayout.bytesPerRow = pixelSize * textureDesc.size.width;
    dataLayout.rowsPerImage = textureDesc.size.height;

    wgpuQueueWriteTexture(nk::webgpu::instance()->queue, &copyTextureDesc,
                          pixels, imageDataSize, &dataLayout,
                          &textureDesc.size);

    image->width = (float)width;
    image->height = (float)height;

    WGPUTextureViewDescriptor textureViewDesc{};
    textureViewDesc.nextInChain = nullptr;
    textureViewDesc.label = "Texture";
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.aspect = WGPUTextureAspect_All;
    image->textureView =
        wgpuTextureCreateView(image->texture, &textureViewDesc);

    WGPUBindGroupEntry textureEntry{};
    textureEntry.nextInChain = nullptr;
    textureEntry.binding = 0;
    textureEntry.buffer = nullptr;
    textureEntry.offset = 0;
    textureEntry.size = 0;
    textureEntry.sampler = nullptr;
    textureEntry.textureView = image->textureView;

    WGPUBindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.label = "TextureBindGroup";
    bindGroupDesc.layout = canvas->spriteBindGroupLayout[1];
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &textureEntry;
    image->bindGroup = wgpuDeviceCreateBindGroup(nk::webgpu::instance()->device,
                                                 &bindGroupDesc);

    return image;
}

NkImage* nk::canvas::createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                             uint32_t height) {
    NkImage* image = (NkImage*)nk::utils::memZeroAlloc(1, sizeof(NkImage));
    if (!image)
        return nullptr;

    size_t pixelSize = sizeof(uint32_t);
    size_t imageDataSize = pixelSize * width * height;

    WGPUTextureDescriptor textureDesc{};
    textureDesc.nextInChain = nullptr;
    textureDesc.label = "NkImage::renderTarget";
    textureDesc.usage = WGPUTextureUsage_TextureBinding |
                        WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc |
                        WGPUTextureUsage_RenderAttachment;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = {width, height, 1};
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    image->texture =
        wgpuDeviceCreateTexture(nk::webgpu::instance()->device, &textureDesc);

    image->width = (float)width;
    image->height = (float)height;

    WGPUTextureViewDescriptor textureViewDesc{};
    textureViewDesc.nextInChain = nullptr;
    textureViewDesc.label = "RenderTargetView";
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.aspect = WGPUTextureAspect_All;
    image->textureView =
        wgpuTextureCreateView(image->texture, &textureViewDesc);

    WGPUBindGroupEntry textureEntry{};
    textureEntry.nextInChain = nullptr;
    textureEntry.binding = 0;
    textureEntry.buffer = nullptr;
    textureEntry.offset = 0;
    textureEntry.size = 0;
    textureEntry.sampler = nullptr;
    textureEntry.textureView = image->textureView;

    WGPUBindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.label = "TextureBindGroup";
    bindGroupDesc.layout = canvas->spriteBindGroupLayout[1];
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &textureEntry;
    image->bindGroup = wgpuDeviceCreateBindGroup(nk::webgpu::instance()->device,
                                                 &bindGroupDesc);

    return image;
}

bool nk::canvas::destroyImage(NkCanvas* canvas, NkImage* image) {
    if (image) {
        wgpuTextureDestroy(image->texture);
        nk::utils::memFree(image);
        return true;
    }
    return false;
}

float nk::img::width(NkImage* image) { return image->width; }

float nk::img::height(NkImage* image) { return image->height; }

void nk::webgpu::drawFrame(NkCanvas* canvas, uint64_t currentFrameIndex) {
    WGPUDevice device = nk::webgpu::instance()->device;
    WGPUQueue queue = nk::webgpu::instance()->queue;
    float viewWidth = canvas->app->windowWidth;
    float viewHeight = canvas->app->windowHeight;

    if (canvas->renderTarget) {
        viewWidth = canvas->renderTarget->width;
        viewHeight = canvas->renderTarget->height;
    }
    // Update Resolution
    float resolution[] = {viewWidth, viewHeight};
    wgpuQueueWriteBuffer(nk::webgpu::instance()->queue,
                         canvas->resolutionBuffer, 0, resolution,
                         sizeof(float) * 2);

    // Upload vertex buffer data
    NkCanvasDrawBatchInternalArray& drawBatchArray =
        canvas->base.drawBatchArray[currentFrameIndex];
    for (uint32_t index = 0; index < drawBatchArray.drawBatchNum; ++index) {
        NkCanvasDrawBatchInternal& drawBatch =
            drawBatchArray.drawBatches[index];
        NkCanvasVertexBuffer* vertexBuffer = drawBatch.buffer;
        NkWebGPUBuffer* buffer = (NkWebGPUBuffer*)vertexBuffer->gpuVertexBuffer;
        wgpuQueueWriteBuffer(queue, buffer->buffer, 0, vertexBuffer->vertices,
                             vertexBuffer->vertexCount *
                                 sizeof(NkCanvasVertex));
    }

    WGPUCommandEncoderDescriptor commandEncoderDesc{};
    commandEncoderDesc.nextInChain = nullptr;
    commandEncoderDesc.label = "CanvasCommandEncoder";
    WGPUCommandEncoder commandEncoder =
        wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    nk::webgpu::updateTextureAtlas(canvas->base.frameTextureAtlas,
                                   commandEncoder);
#endif

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.nextInChain = nullptr;
    if (canvas->renderTarget) {
        colorAttachment.view = canvas->renderTarget->textureView;
    } else {
        colorAttachment.view =
            wgpuSwapChainGetCurrentTextureView(canvas->swapChain);
        ;
    }
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue.r = (double)canvas->clearColor[0];
    colorAttachment.clearValue.g = (double)canvas->clearColor[1];
    colorAttachment.clearValue.b = (double)canvas->clearColor[2];
    colorAttachment.clearValue.a = (double)canvas->clearColor[3];

    WGPURenderPassDescriptor renderPassDesc{};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.label = "CanvasRenderPass";
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.occlusionQuerySet = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder =
        wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDesc);

    {
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, canvas->spritePSO);
        NkWebGPUBuffer* indexBuffer =
            (NkWebGPUBuffer*)canvas->base.gpuIndexBuffer;
        wgpuRenderPassEncoderSetIndexBuffer(
            renderPassEncoder, indexBuffer->buffer, WGPUIndexFormat_Uint32, 0,
            NK_CANVAS_MAX_INDICES_BYTE_SIZE);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0, 0, viewWidth,
                                         viewHeight, 0.0f, 1.0f);
        wgpuRenderPassEncoderSetScissorRect(renderPassEncoder, 0, 0, viewWidth,
                                            viewHeight);
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0,
                                          canvas->staticBindGroup, 0, nullptr);

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
        wgpuRenderPassEncoderSetBindGroup(
            renderPassEncoder, 1,
            ((NkTextureAtlasResource*)canvas->base.frameTextureAtlas.gpuTexture)
                ->bindGroup,
            0, nullptr);
#endif

        for (uint32_t index = 0; index < drawBatchArray.drawBatchNum; ++index) {
            NkCanvasDrawBatchInternal& drawBatch =
                drawBatchArray.drawBatches[index];
            NkWebGPUBuffer* vertexBuffer =
                (NkWebGPUBuffer*)drawBatch.buffer->gpuVertexBuffer;
            wgpuRenderPassEncoderSetVertexBuffer(
                renderPassEncoder, 0, vertexBuffer->buffer, 0,
                drawBatch.buffer->vertexCount * sizeof(NkCanvasVertex));
#if !NK_CANVAS_TEXTURE_ATLAS_ENABLED
            wgpuRenderPassEncoderSetBindGroup(
                renderPassEncoder, 1, drawBatch.image->bindGroup, 0, nullptr);
#endif
            wgpuRenderPassEncoderDrawIndexed(renderPassEncoder, drawBatch.count,
                                             1, 0, drawBatch.bufferOffset, 0);
        }
    }
    wgpuRenderPassEncoderEnd(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferEncoder{};
    commandBufferEncoder.nextInChain = nullptr;
    commandBufferEncoder.label = "CanvasCommandBuffer";
    WGPUCommandBuffer commandBuffer =
        wgpuCommandEncoderFinish(commandEncoder, &commandBufferEncoder);
    wgpuQueueSubmit(queue, 1, &commandBuffer);

    wgpuCommandEncoderRelease(commandEncoder);
    wgpuCommandBufferRelease(commandBuffer);
}

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED

void nk::canvas_internal::initTextureAtlasResource(
    NkCanvas* canvas, NkTextureAtlas& textureAtlas) {
    NkTextureAtlasResource* texture =
        (NkTextureAtlasResource*)nk::utils::memZeroAlloc(
            1, sizeof(NkTextureAtlasResource));
    if (!texture)
        return;

    size_t pixelSize = sizeof(uint32_t);
    size_t imageDataSize = pixelSize * textureAtlas.width * textureAtlas.height;

    WGPUTextureDescriptor textureDesc{};
    textureDesc.nextInChain = nullptr;
    textureDesc.label = "NkTextureAtlas::texture";
    textureDesc.usage =
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = {textureAtlas.width, textureAtlas.height, 1};
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    texture->texture =
        wgpuDeviceCreateTexture(nk::webgpu::instance()->device, &textureDesc);

    WGPUTextureViewDescriptor textureViewDesc{};
    textureViewDesc.nextInChain = nullptr;
    textureViewDesc.label = "TextureAtlasView";
    textureViewDesc.format = textureDesc.format;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.aspect = WGPUTextureAspect_All;
    texture->textureView =
        wgpuTextureCreateView(texture->texture, &textureViewDesc);

    WGPUBindGroupEntry textureEntry{};
    textureEntry.nextInChain = nullptr;
    textureEntry.binding = 0;
    textureEntry.buffer = nullptr;
    textureEntry.offset = 0;
    textureEntry.size = 0;
    textureEntry.sampler = nullptr;
    textureEntry.textureView = texture->textureView;

    WGPUBindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.label = "TextureAtlasBindGroup";
    bindGroupDesc.layout = canvas->spriteBindGroupLayout[1];
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &textureEntry;
    texture->bindGroup = wgpuDeviceCreateBindGroup(
        nk::webgpu::instance()->device, &bindGroupDesc);

    textureAtlas.gpuTexture = (void*)texture;
}

void nk::canvas_internal::destroyTextureAtlasResource(
    NkTextureAtlas& textureAtlas) {
    NkTextureAtlasResource* texture =
        (NkTextureAtlasResource*)textureAtlas.gpuTexture;
    wgpuTextureDestroy(texture->texture);
    wgpuTextureViewRelease(texture->textureView);
    wgpuBindGroupRelease(texture->bindGroup);
    nk::utils::memFree(texture);
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

void nk::webgpu::updateTextureAtlas(NkTextureAtlas& textureAtlas,
                                    WGPUCommandEncoder commandEncoder) {
    WGPUQueue queue = nk::webgpu::instance()->queue;
    NkTextureAtlasResource* texture =
        (NkTextureAtlasResource*)textureAtlas.gpuTexture;
    WGPUImageCopyTexture destImageCopy{};
    destImageCopy.nextInChain = nullptr;
    destImageCopy.texture = texture->texture;
    destImageCopy.mipLevel = 0;
    destImageCopy.origin = {0, 0, 0};
    destImageCopy.aspect = WGPUTextureAspect_All;

    for (auto& entry : textureAtlas.images) {
        NkImage* image = entry.first;
        NkTextureAtlasRect& rect = entry.second;
        WGPUImageCopyTexture srcImageCopy{};
        srcImageCopy.nextInChain = nullptr;
        srcImageCopy.texture = image->texture;
        srcImageCopy.mipLevel = 0;
        srcImageCopy.origin = {0, 0, 0};
        srcImageCopy.aspect = WGPUTextureAspect_All;

        WGPUExtent3D extent = {(uint32_t)image->width, (uint32_t)image->height,
                               1};

        destImageCopy.origin.x = rect.x;
        destImageCopy.origin.y = rect.y;
        wgpuCommandEncoderCopyTextureToTexture(commandEncoder, &srcImageCopy,
                                               &destImageCopy, &extent);
        image->state &= ~NK_IMAGE_BIT_TEXTURE_ATLAS;
    }
}

#endif
