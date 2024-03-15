#include "windows_canvas_d3d12.h"
#include "sprite_shaders.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

static NkD3D12Instance d3d12Instance = {};
extern NkAppResizeInfo internalResizeInfo;

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
static const char spriteVS[] = R"(
const float2 resolution : register(b0);

struct VertexIn {
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
	uint textureId : TEXCOORD1;
};

struct VertexOut {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
	uint textureId : TEXCOORD1;
};

VertexOut main(VertexIn vtx) {
	VertexOut vtxOut;
	vtxOut.position = float4((vtx.position / resolution) * 2.0 - 1.0, 0.0, 1.0);
	vtxOut.position.y = -vtxOut.position.y;
	vtxOut.texCoord = vtx.texCoord;
	vtxOut.color = vtx.color;
	vtxOut.textureId = vtx.textureId;
	return vtxOut;
}

)";
static const char spritePS[] = R"(
SamplerState samplerLinear : register(s0);
SamplerState samplerPoint  : register(s1);

Texture2D<float4> mainTexture[] : register(t0);
struct VertexOut {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
	uint textureId : TEXCOORD1;
};

float4 main(VertexOut vtx) : SV_TARGET {
	return mainTexture[vtx.textureId].SampleLevel(samplerPoint, vtx.texCoord, 0) * vtx.color;
}
)";
#else
static const char spriteVS[] = R"(
const float2 resolution : register(b0);

struct VertexIn {
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;

};

struct VertexOut {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
};

VertexOut main(VertexIn vtx) {
	VertexOut vtxOut;
	vtxOut.position = float4((vtx.position / resolution) * 2.0 - 1.0, 0.0, 1.0);
	vtxOut.position.y = -vtxOut.position.y;
	vtxOut.texCoord = vtx.texCoord;
	vtxOut.color = vtx.color;
	return vtxOut;
}

)";
static const char spritePS[] = R"(
SamplerState samplerLinear : register(s0);
SamplerState samplerPoint  : register(s1);

Texture2D<float4> mainTexture : register(t0);
struct VertexOut { float4 position : SV_POSITION; float2 texCoord : TEXCOORD; float4 color : COLOR; };
float4 main(VertexOut vtx) : SV_TARGET {
	return mainTexture.Sample(samplerPoint, vtx.texCoord) * vtx.color;
}
)";
#endif

// Source: https://devblogs.microsoft.com/pix/taking-a-capture/
static void loadPIX() {
    if (GetModuleHandleA("WinPixGpuCapture.dll") == 0) {

        LPWSTR programFilesPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL,
                             &programFilesPath);

        std::wstring pixSearchPath =
            programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

        WIN32_FIND_DATAW findData;
        bool foundPixInstallation = false;
        wchar_t newestVersionFound[MAX_PATH];

        HANDLE hFind = FindFirstFileW(pixSearchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
                     FILE_ATTRIBUTE_DIRECTORY) &&
                    (findData.cFileName[0] != '.')) {
                    if (!foundPixInstallation ||
                        wcscmp(newestVersionFound, findData.cFileName) <= 0) {
                        foundPixInstallation = true;
                        StringCchCopyW(newestVersionFound,
                                       _countof(newestVersionFound),
                                       findData.cFileName);
                    }
                }
            } while (FindNextFileW(hFind, &findData) != 0);
        }

        FindClose(hFind);

        if (foundPixInstallation) {
            wchar_t output[MAX_PATH];
            StringCchCopyW(output, pixSearchPath.length(),
                           pixSearchPath.data());
            StringCchCatW(output, MAX_PATH, &newestVersionFound[0]);
            StringCchCatW(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");
            LoadLibraryW(output);
        }
    }
}

NkD3D12Instance* nk::d3d12::createInstance(NkApp* app) {

    if (!d3d12Instance.device) {
#if NK_DEBUG
        loadPIX();
        D3D_ASSERT(
            D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Instance.debugInterface)),
            "Failed to create debug interface");
        d3d12Instance.debugInterface->EnableDebugLayer();
        d3d12Instance.debugInterface->SetEnableGPUBasedValidation(true);
        UINT factoryFlag = DXGI_CREATE_FACTORY_DEBUG;
#else
        UINT factoryFlag = 0;
#endif

        D3D_ASSERT(CreateDXGIFactory2(factoryFlag,
                                      IID_PPV_ARGS(&d3d12Instance.factory)),
                   "Failed to create factory");
        D3D_ASSERT(d3d12Instance.factory->EnumAdapters(
                       0, (IDXGIAdapter**)(&d3d12Instance.adapter)),
                   "Failed to aquire adapter");
        D3D_ASSERT(D3D12CreateDevice((IUnknown*)d3d12Instance.adapter,
                                     D3D_FEATURE_LEVEL_12_0,
                                     IID_PPV_ARGS(&d3d12Instance.device)),
                   "Failed to create device");
    }

    d3d12Instance.refCount++;

    return &d3d12Instance;
}

void nk::d3d12::destroyInstance(NkD3D12Instance* instance) {
    if (--d3d12Instance.refCount == 0 && d3d12Instance.device != nullptr) {
        D3D_RELEASE(d3d12Instance.device);
        D3D_RELEASE(d3d12Instance.factory);
        D3D_RELEASE(d3d12Instance.adapter);
        if (d3d12Instance.debugInterface) {
            IDXGIDebug1* dxgiDebug = nullptr;
            if (DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)) == S_OK) {
                dxgiDebug->ReportLiveObjects(
                    DXGI_DEBUG_ALL,
                    DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY |
                                         DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
            D3D_RELEASE(d3d12Instance.debugInterface);
        }
    }
}

NkD3D12Instance* nk::d3d12::instance() { return &d3d12Instance; }

ID3D12Resource*
nk::d3d12::createNativeBuffer(size_t bufferSize, NkBufferType bufferType,
                              const wchar_t* name,
                              D3D12_RESOURCE_STATES* outInitState) {
    D3D12_HEAP_TYPE heapType;
    D3D12_RESOURCE_STATES initialState;
    D3D12_RESOURCE_FLAGS flags;
    switch (bufferType) {
    case NkBufferType::INDEX_BUFFER:
        initialState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        flags = D3D12_RESOURCE_FLAG_NONE;
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        break;
    case NkBufferType::VERTEX_BUFFER:
        initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        flags = D3D12_RESOURCE_FLAG_NONE;
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        break;
    case NkBufferType::CONSTANT_BUFFER:
        initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        flags = D3D12_RESOURCE_FLAG_NONE;
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        break;
    case NkBufferType::UNORDERED_BUFFER:
        initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        break;
    case NkBufferType::UPLOAD_BUFFER:
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        flags = D3D12_RESOURCE_FLAG_NONE;
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        break;
    case NkBufferType::SHADER_RESOURCE_BUFFER:
        initialState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        flags = D3D12_RESOURCE_FLAG_NONE;
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        break;
    default:
        NK_PANIC("Error: Invalid buffer type"); // Invalid Buffer Type
        break;
    }

    D3D12_RESOURCE_DESC resourceDesc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = nk::utils::alignSize(bufferSize, 256),
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = flags};
    D3D12_HEAP_PROPERTIES heapProps = {
        .Type = heapType,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0};

    ID3D12Resource* resource = nullptr;
    D3D_ASSERT(nk::d3d12::instance()->device->CreateCommittedResource(
                   &heapProps,
                   D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
                       D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                   &resourceDesc, initialState, nullptr,
                   IID_PPV_ARGS(&resource)),
               "Failed to create buffer resource");
    resource->SetName(name);
    if (outInitState) {
        *outInitState = initialState;
    }
    return resource;
}

NkD3D12Resource* nk::d3d12::createBuffer(size_t bufferSize,
                                         NkBufferType bufferType,
                                         const wchar_t* name) {
    NkD3D12Resource* buffer =
        (NkD3D12Resource*)nk::utils::memZeroAlloc(1, sizeof(NkD3D12Resource));
    if (!buffer)
        return nullptr;
    buffer->resource =
        createNativeBuffer(bufferSize, bufferType, name, &buffer->state);
    buffer->desc = buffer->resource->GetDesc();
    return buffer;
}

NkD3D12DynamicBuffer* nk::d3d12::createDynamicBuffer(size_t bufferSize,
                                                     NkBufferType bufferType,
                                                     const wchar_t* name) {
    NkD3D12DynamicBuffer* buffer =
        (NkD3D12DynamicBuffer*)nk::utils::memZeroAlloc(
            1, sizeof(NkD3D12DynamicBuffer));
    if (!buffer)
        return nullptr;
    buffer->resource.resource = createNativeBuffer(bufferSize, bufferType, name,
                                                   &buffer->resource.state);
    buffer->resource.desc = buffer->resource.resource->GetDesc();
    buffer->uploadBuffer =
        createNativeBuffer(bufferSize, NkBufferType::UPLOAD_BUFFER,
                           L"DynamicUploadBuffer", nullptr);
    return buffer;
}

NkImage* nk::d3d12::createImage(NkCanvas* canvas, uint32_t width,
                                uint32_t height, NkImageFormat format,
                                const void* pixels) {
    NkImage* image = (NkImage*)nk::utils::memZeroAlloc(1, sizeof(NkImage));
    if (!image)
        return nullptr;
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_STATES initialState =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    D3D12_RESOURCE_DESC resourceDesc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = (uint64_t)width,
        .Height = height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = dxgiFormat,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE};
    D3D12_HEAP_PROPERTIES heapProps = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0};
    D3D_ASSERT(nk::d3d12::instance()->device->CreateCommittedResource(
                   &heapProps,
                   D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
                       D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                   &resourceDesc, initialState, nullptr,
                   IID_PPV_ARGS(&image->buffer.resource.resource)),
               "Failed to create image resource");
    image->buffer.resource.state = initialState;
    image->buffer.resource.desc = image->buffer.resource.resource->GetDesc();
    image->width = (float)width;
    image->height = (float)height;
    image->buffer.resource.resource->SetName(L"NkImage::buffer");

    size_t pixelSize = nk::d3d12::getDXGIFormatBytes(dxgiFormat);
    uint64_t alignedWidth =
        nk::utils::alignSize(width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    uint64_t dataSize = alignedWidth * height * pixelSize;
    uint64_t bufferSize = dataSize < 256 ? 256 : dataSize;
    image->buffer.uploadBuffer =
        nk::d3d12::createNativeBuffer(bufferSize, NkBufferType::UPLOAD_BUFFER,
                                      L"NkImage::uploadBuffer", nullptr);
    image->buffer.uploadBuffer->SetName(L"NkImage::uploadBuffer");
    image->state = 0;
    image->cpuData = nk::utils::memRealloc(nullptr, pixelSize * width * height);
    if (image->cpuData) {
        memcpy(image->cpuData, pixels, pixelSize * width * height);
    } else {
        NK_PANIC("Error: Failed to allocate CPU memory for image.");
    }
    processImage(canvas, image);
    return image;
}

NkImage* nk::d3d12::createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                            uint32_t height) {
    NkImage* image = (NkImage*)nk::utils::memZeroAlloc(1, sizeof(NkImage));
    if (!image)
        return nullptr;
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_STATES initialState =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    D3D12_RESOURCE_DESC resourceDesc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = (uint64_t)width,
        .Height = height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = dxgiFormat,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET};
    D3D12_HEAP_PROPERTIES heapProps = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0};
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    D3D_ASSERT(nk::d3d12::instance()->device->CreateCommittedResource(
                   &heapProps,
                   D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
                       D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                   &resourceDesc, initialState, &clearValue,
                   IID_PPV_ARGS(&image->buffer.resource.resource)),
               "Failed to create image resource");
    image->buffer.resource.state = initialState;
    image->buffer.resource.desc = image->buffer.resource.resource->GetDesc();
    image->width = (float)width;
    image->height = (float)height;
    image->buffer.resource.resource->SetName(L"NkImage::renderTarget");
    return image;
}

void nk::d3d12::destroyImage(NkImage* image) {
    if (image) {
        D3D_RELEASE(image->buffer.resource.resource);
        D3D_RELEASE(image->buffer.uploadBuffer);
        nk::utils::memFree(image->cpuData);
        nk::utils::memFree(image);
    }
}

size_t nk::d3d12::getDXGIFormatBits(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    default:
        return 0;
    }
}

size_t nk::d3d12::getDXGIFormatBytes(DXGI_FORMAT format) {
    return getDXGIFormatBits(format) / 8;
}

void NkD3D12DescriptorAllocator::init(uint32_t descriptorNum,
                                      D3D12_DESCRIPTOR_HEAP_TYPE type) {
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.Type = type;
    descriptorHeapDesc.NumDescriptors = descriptorNum;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    D3D_ASSERT(nk::d3d12::instance()->device->CreateDescriptorHeap(
                   &descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)),
               "Error: Failed to create descriptor heap");
    descriptorHandleSize =
        nk::d3d12::instance()->device->GetDescriptorHandleIncrementSize(type);
    descriptorAllocated = 0;
    gpuBaseHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    cpuBaseHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void NkD3D12DescriptorAllocator::destroy() { D3D_RELEASE(descriptorHeap); }

void NkD3D12DescriptorAllocator::reset() { descriptorAllocated = 0; }

NkD3D12DescriptorTable
NkD3D12DescriptorAllocator::allocate(uint32_t descriptorNum) {
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {
        gpuBaseHandle.ptr + (descriptorAllocated * descriptorHandleSize)};
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {
        cpuBaseHandle.ptr + (descriptorAllocated * descriptorHandleSize)};
    descriptorAllocated += descriptorNum;
    return {gpuHandle, cpuHandle, descriptorHandleSize};
}

void NkD3D12Fence::init(ID3D12Device* device, uint64_t initialValue) {
    device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE,
                        IID_PPV_ARGS(&fence));
    event = CreateEvent(nullptr, false, false, nullptr);
}

void NkD3D12Fence::destroy() {
    D3D_RELEASE(fence);
    CloseHandle(event);
}

void NkD3D12Fence::waitForValue(uint64_t waitValue) {
    if (fence->GetCompletedValue() != waitValue) {
        fence->SetEventOnCompletion(waitValue, event);
        WaitForSingleObject(event, INFINITE);
    }
}

void NkImageArray::init() {
    images = (NkImage**)nk::utils::memRealloc(nullptr, sizeof(NkImage*) * 16);
    imageNum = 0;
    imageMax = 16;
}

void NkImageArray::destroy() { nk::utils::memFree(images); }

void NkImageArray::reset() { imageNum = 0; }

void NkImageArray::add(NkImage* image) {
    if (imageNum + 1 > imageMax) {
        imageMax *= 2;
        images = (NkImage**)nk::utils::memRealloc(images,
                                                  sizeof(NkImage*) * imageMax);
    }
    if (images) {
        images[imageNum++] = image;
    }
}

void nk::canvas_internal::initVertexBuffer(NkCanvasVertexBuffer* vertexBuffer,
                                           size_t bufferSize) {
    NkD3D12DynamicBuffer* buffer = nk::d3d12::createDynamicBuffer(
        bufferSize, NkBufferType::VERTEX_BUFFER, L"NkCanvas::vertexBuffer");
    if (buffer) {
        vertexBuffer->gpuVertexBuffer = (void*)buffer;
    }
}

void nk::canvas_internal::destroyVertexBuffer(
    NkCanvasVertexBuffer* vertexBuffer) {
    if (vertexBuffer && vertexBuffer->gpuVertexBuffer) {
        NkD3D12DynamicBuffer* buffer =
            (NkD3D12DynamicBuffer*)vertexBuffer->gpuVertexBuffer;
        D3D_RELEASE(buffer->resource.resource);
        D3D_RELEASE(buffer->uploadBuffer);
        nk::utils::memFree(buffer);
        vertexBuffer->gpuVertexBuffer = nullptr;
    }
}

void nk::canvas_internal::initIndexBuffer(void** gpuIndexBuffer) {
    NkD3D12DynamicBuffer* buffer = nk::d3d12::createDynamicBuffer(
        NK_CANVAS_MAX_INDICES_BYTE_SIZE, NkBufferType::INDEX_BUFFER,
        L"NkCanvas::indexBuffer");
    if (buffer) {
        *gpuIndexBuffer = (void*)buffer;
    }
}

void nk::canvas_internal::destroyIndexBuffer(void** gpuIndexBuffer) {
    if (gpuIndexBuffer && *gpuIndexBuffer) {
        NkD3D12DynamicBuffer* buffer = (NkD3D12DynamicBuffer*)*gpuIndexBuffer;
        D3D_RELEASE(buffer->resource.resource);
        D3D_RELEASE(buffer->uploadBuffer);
        nk::utils::memFree(buffer);
        *gpuIndexBuffer = nullptr;
    }
}

void nk::canvas_internal::signalFrameSyncPoint(NkCanvas* canvas,
                                               NkGPUHandle gpuSyncPoint,
                                               uint64_t value) {
    NkD3D12Fence* fence = (NkD3D12Fence*)gpuSyncPoint;
    canvas->commandQueue->Signal(fence->fence, value);
}

void nk::canvas_internal::waitFrameSyncPoint(NkGPUHandle gpuSyncPoint,
                                             uint64_t value) {
    NkD3D12Fence* fence = (NkD3D12Fence*)gpuSyncPoint;
    uint64_t lastValue = fence->fence->GetCompletedValue();
    fence->waitForValue(value);
}

void nk::canvas_internal::initFrameSyncPoint(NkGPUHandle* gpuSyncPoint) {
    NkD3D12Fence* fence =
        (NkD3D12Fence*)nk::utils::memZeroAlloc(1, sizeof(NkD3D12Fence));
    if (fence) {
        fence->init(nk::d3d12::instance()->device);
        *gpuSyncPoint = (NkGPUHandle)fence;
    }
}

void nk::canvas_internal::destroyFrameSyncPoint(NkGPUHandle* gpuSyncPoint) {
    if (gpuSyncPoint && *gpuSyncPoint) {
        NkD3D12Fence* fence = (NkD3D12Fence*)*gpuSyncPoint;
        fence->destroy();
        nk::utils::memFree(fence);
        *gpuSyncPoint = nullptr;
    }
}

NkCanvasBase* nk::canvas_internal::canvasBase(NkCanvas* canvas) {
    return &canvas->base;
}

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
void nk::canvas_internal::initTextureAtlasResource(
    NkCanvas* canvas, NkTextureAtlas& textureAtlas) {
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_RESOURCE_STATES initialState =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    D3D12_RESOURCE_DESC resourceDesc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = (uint64_t)textureAtlas.width,
        .Height = textureAtlas.height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = dxgiFormat,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE};
    D3D12_HEAP_PROPERTIES heapProps = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0};
    ID3D12Resource* resource = nullptr;
    D3D_ASSERT(nk::d3d12::instance()->device->CreateCommittedResource(
                   &heapProps,
                   D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
                       D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                   &resourceDesc, initialState, nullptr,
                   IID_PPV_ARGS(&resource)),
               "Failed to create image resource for texture atlas");
    textureAtlas.gpuTexture = (void*)resource;
}

void nk::canvas_internal::destroyTextureAtlasResource(
    NkTextureAtlas& textureAtlas) {
    ID3D12Resource* resource = (ID3D12Resource*)textureAtlas.gpuTexture;
    D3D_RELEASE(resource);
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

#endif

NkCanvas* nk::canvas::create(NkApp* app, bool allowResize) {
    NkCanvas* canvas = (NkCanvas*)nk::utils::memZeroAlloc(1, sizeof(NkCanvas));
    if (!canvas)
        return nullptr;
    canvas->app = app;

    nk::d3d12::createInstance(app);
    ID3D12Device* device = nk::d3d12::instance()->device;

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = 0;
    D3D_ASSERT(device->CreateCommandQueue(&commandQueueDesc,
                                          IID_PPV_ARGS(&canvas->commandQueue)),
               "Error: Failed to initialize command queue.");
    canvas->base.init(canvas, (float)app->windowWidth,
                      (float)app->windowHeight);
    for (uint32_t index = 0; index < NK_CANVAS_MAX_FRAMES; ++index) {
        D3D_ASSERT(
            device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&canvas->commandLists[index].commandAllocator)),
            "");
        canvas->commandLists[index].commandAllocator->SetName(
            L"NkCanvas::commandAllocator");
        D3D_ASSERT(device->CreateCommandList(
                       0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                       canvas->commandLists[index].commandAllocator, nullptr,
                       IID_PPV_ARGS(&canvas->commandLists[index].commandList)),
                   "");
        canvas->commandLists[index].commandList->SetName(
            L"NkCanvas::commandList");
        D3D_ASSERT(canvas->commandLists[index].commandList->Close(),
                   "Error: Failed to close commandlist.");
        canvas->descriptorAllocators[index].init(
            NK_CANVAS_D3D12_MAX_SRV_DESCRIPTORS_PER_FRAME,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        canvas->imageUploads[index].init();
        canvas->imagesToDestroy[index].init();
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {
        .BufferDesc = {.Width = app->windowWidth,
                       .Height = app->windowHeight,
                       .RefreshRate = {.Numerator = 0, .Denominator = 0},
                       .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                       .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                       .Scaling = DXGI_MODE_SCALING_UNSPECIFIED},
        .SampleDesc = {.Count = 1, .Quality = 0},
        .BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT,
        .BufferCount = NK_CANVAS_D3D12_MAX_BACKBUFFERS,
        .OutputWindow = app->windowHandle,
        .Windowed = app->windowed,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH};

    D3D_ASSERT(nk::d3d12::instance()->factory->CreateSwapChain(
                   (IUnknown*)canvas->commandQueue, &swapChainDesc,
                   (IDXGISwapChain**)&canvas->swapChain),
               "Error: Failed to create swapchain");
    for (uint32_t index = 0; index < NK_CANVAS_D3D12_MAX_BACKBUFFERS; ++index) {
        ID3D12Resource* resource = nullptr;
        canvas->swapChain->GetBuffer(index, IID_PPV_ARGS(&resource));
        canvas->backbuffers[index] = resource;
        resource->SetName(L"NkCanvas::backbuffer");
    }

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.NumDescriptors = NK_CANVAS_D3D12_MAX_BACKBUFFERS;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    D3D_ASSERT(
        device->CreateDescriptorHeap(&descriptorHeapDesc,
                                     IID_PPV_ARGS(&canvas->rtvDescriptorHeap)),
        "Error: Failed to create RTV descriptor heap");

    canvas->base.indicesUploaded = false;

#if !NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    ID3DBlob *vsBlob, *psBlob, *errorBlob;
    if (D3DCompile(spriteVS, strlen(spriteVS), nullptr, nullptr, nullptr,
                   "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob) != S_OK) {
        NK_LOG("Error: Failed to compile vertex shader.\n%s",
               errorBlob ? errorBlob->GetBufferPointer() : "");
        NK_PANIC("error");
    }

    if (D3DCompile(spritePS, strlen(spritePS), nullptr, nullptr, nullptr,
                   "main", "ps_5_0", 0, 0, &psBlob, &errorBlob) != S_OK) {
        NK_LOG("Error: Failed to compile pixel shader.\n%s",
               errorBlob ? errorBlob->GetBufferPointer() : "");
        NK_PANIC("error");
    }
#endif

    D3D12_ROOT_PARAMETER rootParameters[2] = {};

    // resolution
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Constants.Num32BitValues = 2;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.ShaderRegister = 0;

    // mainTexture
    D3D12_DESCRIPTOR_RANGE mainTextureRange[1] = {};
    mainTextureRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    mainTextureRange[0].NumDescriptors =
        NK_CANVAS_D3D12_MAX_SRV_DESCRIPTORS_PER_FRAME;
#else
    mainTextureRange[0].NumDescriptors = 1;
#endif
    mainTextureRange[0].BaseShaderRegister = 0;
    mainTextureRange[0].RegisterSpace = 0;
    mainTextureRange[0].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[1].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = mainTextureRange;

    D3D12_STATIC_SAMPLER_DESC staticSamplerDesc[2] = {};

    // samplerLinear
    staticSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplerDesc[0].MipLODBias = 0.0f;
    staticSamplerDesc[0].MaxAnisotropy = 0;
    staticSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSamplerDesc[0].BorderColor =
        D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplerDesc[0].MinLOD = 0;
    staticSamplerDesc[0].MaxLOD = 0;
    staticSamplerDesc[0].ShaderRegister = 0;
    staticSamplerDesc[0].RegisterSpace = 0;
    staticSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // samplerPoint
    staticSamplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplerDesc[1].MipLODBias = 0.0f;
    staticSamplerDesc[1].MaxAnisotropy = 0;
    staticSamplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSamplerDesc[1].BorderColor =
        D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplerDesc[1].MinLOD = 0;
    staticSamplerDesc[1].MaxLOD = 0;
    staticSamplerDesc[1].ShaderRegister = 1;
    staticSamplerDesc[1].RegisterSpace = 0;
    staticSamplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 2;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 2;
    rootSignatureDesc.pStaticSamplers = staticSamplerDesc;
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* rootSignatureBlob;
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    ID3DBlob* errorBlob;
#endif
    if (D3D12SerializeRootSignature(&rootSignatureDesc,
                                    D3D_ROOT_SIGNATURE_VERSION_1,
                                    &rootSignatureBlob, &errorBlob) != S_OK) {
        NK_LOG("Error: Failed to serialize root signature.\n%s",
               errorBlob ? errorBlob->GetBufferPointer() : "");
        NK_PANIC("error");
    }
    D3D_ASSERT(
        device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
                                    rootSignatureBlob->GetBufferSize(),
                                    IID_PPV_ARGS(&canvas->spriteRootSignature)),
        "Error: Failed to create root signature");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC spritePSODesc{};
    spritePSODesc.pRootSignature = canvas->spriteRootSignature;
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    spritePSODesc.VS = {sprite_vs, sizeof(sprite_vs)};
    spritePSODesc.PS = {sprite_ps, sizeof(sprite_ps)};
#else
    spritePSODesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
    spritePSODesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
#endif
    spritePSODesc.BlendState.AlphaToCoverageEnable = false;
    spritePSODesc.BlendState.IndependentBlendEnable = false;
    spritePSODesc.BlendState.RenderTarget[0].BlendEnable = true;
    spritePSODesc.BlendState.RenderTarget[0].LogicOpEnable = false;
    spritePSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    spritePSODesc.BlendState.RenderTarget[0].DestBlend =
        D3D12_BLEND_INV_SRC_ALPHA;
    spritePSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    spritePSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    spritePSODesc.BlendState.RenderTarget[0].DestBlendAlpha =
        D3D12_BLEND_INV_SRC_ALPHA;
    spritePSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    spritePSODesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    spritePSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;
    spritePSODesc.SampleMask = UINT_MAX;
    spritePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    spritePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    spritePSODesc.RasterizerState.FrontCounterClockwise = !false;
    spritePSODesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    spritePSODesc.RasterizerState.DepthBiasClamp =
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    spritePSODesc.RasterizerState.SlopeScaledDepthBias =
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    spritePSODesc.RasterizerState.DepthClipEnable = false;
    spritePSODesc.RasterizerState.MultisampleEnable = false;
    spritePSODesc.RasterizerState.AntialiasedLineEnable = false;
    spritePSODesc.RasterizerState.ForcedSampleCount = 0;
    spritePSODesc.RasterizerState.ConservativeRaster =
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    spritePSODesc.DepthStencilState.DepthEnable = false;
    spritePSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    spritePSODesc.DepthStencilState.DepthFunc =
        D3D12_COMPARISON_FUNC_LESS_EQUAL;
    spritePSODesc.DepthStencilState.StencilEnable = false;
    spritePSODesc.DepthStencilState.StencilReadMask =
        D3D12_DEFAULT_STENCIL_READ_MASK;
    spritePSODesc.DepthStencilState.StencilWriteMask =
        D3D12_DEFAULT_STENCIL_WRITE_MASK;
    spritePSODesc.DepthStencilState.FrontFace = {
        D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS};
    spritePSODesc.DepthStencilState.BackFace = {
        D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS};
    spritePSODesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    spritePSODesc.NumRenderTargets = 1;
    spritePSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    spritePSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    spritePSODesc.SampleDesc = {.Count = 1, .Quality = 0};
    spritePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[4] = {};
#else
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[3] = {};
#endif
    inputElementDesc[0].SemanticName = "POSITION";
    inputElementDesc[0].SemanticIndex = 0;
    inputElementDesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDesc[0].InputSlot = 0;
    inputElementDesc[0].AlignedByteOffset = offsetof(NkCanvasVertex, position);
    inputElementDesc[0].InputSlotClass =
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDesc[0].InstanceDataStepRate = 0;

    inputElementDesc[1].SemanticName = "TEXCOORD";
    inputElementDesc[1].SemanticIndex = 0;
    inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDesc[1].InputSlot = 0;
    inputElementDesc[1].AlignedByteOffset = offsetof(NkCanvasVertex, texCoord);
    inputElementDesc[1].InputSlotClass =
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDesc[1].InstanceDataStepRate = 0;

    inputElementDesc[2].SemanticName = "COLOR";
    inputElementDesc[2].SemanticIndex = 0;
    inputElementDesc[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    inputElementDesc[2].InputSlot = 0;
    inputElementDesc[2].AlignedByteOffset = offsetof(NkCanvasVertex, color);
    inputElementDesc[2].InputSlotClass =
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDesc[2].InstanceDataStepRate = 0;

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    inputElementDesc[3].SemanticName = "TEXCOORD";
    inputElementDesc[3].SemanticIndex = 1;
    inputElementDesc[3].Format = DXGI_FORMAT_R32_UINT;
    inputElementDesc[3].InputSlot = 0;
    inputElementDesc[3].AlignedByteOffset = offsetof(NkCanvasVertex, textureId);
    inputElementDesc[3].InputSlotClass =
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDesc[3].InstanceDataStepRate = 0;
#endif

    spritePSODesc.InputLayout.pInputElementDescs = inputElementDesc;
    spritePSODesc.InputLayout.NumElements =
        sizeof(inputElementDesc) / sizeof(D3D12_INPUT_ELEMENT_DESC);

    D3D_ASSERT(device->CreateGraphicsPipelineState(
                   &spritePSODesc, IID_PPV_ARGS(&canvas->spritePSO)),
               "Error: Failed to create PSO");

    D3D_RELEASE(rootSignatureBlob);
#if !NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    D3D_RELEASE(vsBlob);
    D3D_RELEASE(psBlob);
#endif
    D3D_RELEASE(errorBlob);

    canvas->allowResize = allowResize;
    canvas->renderTarget = nullptr;

    return canvas;
}

bool nk::canvas::destroy(NkCanvas* canvas) {
    if (canvas) {
        canvas->swapChain->Present(1, 0);
        canvas->base.signalCurrentFrame();
        canvas->base.waitCurrentFrame();
        canvas->base.destroy(canvas);

        for (uint32_t index = 0; index < NK_CANVAS_MAX_FRAMES; ++index) {
            for (uint32_t imgIndex = 0;
                 imgIndex < canvas->imagesToDestroy[index].imageNum;
                 ++imgIndex) {
                nk::d3d12::destroyImage(
                    canvas->imagesToDestroy[index].images[imgIndex]);
            }
            canvas->imagesToDestroy[index].reset();
        }

        D3D_RELEASE(canvas->spriteRootSignature);
        D3D_RELEASE(canvas->spritePSO);

        D3D_RELEASE(canvas->rtvDescriptorHeap);

        for (uint32_t index = 0; index < NK_CANVAS_D3D12_MAX_BACKBUFFERS;
             ++index) {
            D3D_RELEASE(canvas->backbuffers[index]);
        }
        D3D_RELEASE(canvas->swapChain);

        for (uint32_t index = 0; index < NK_CANVAS_MAX_FRAMES; ++index) {
            canvas->descriptorAllocators[index].destroy();
            D3D_RELEASE(canvas->commandLists[index].commandList);
            D3D_RELEASE(canvas->commandLists[index].commandAllocator);
            canvas->imageUploads[index].destroy();
            canvas->imagesToDestroy[index].destroy();
        }
        D3D_RELEASE(canvas->commandQueue);

        nk::d3d12::destroyInstance(nk::d3d12::instance());
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
    nk::d3d12::processImage(canvas, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, uint32_t color,
                           NkImage* image) {
    canvas->base.drawImage(x, y, color, image);
    nk::d3d12::processImage(canvas, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, NkImage* image) {
    canvas->base.drawImage(x, y, width, height, image);
    nk::d3d12::processImage(canvas, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, uint32_t color, NkImage* image) {
    canvas->base.drawImage(x, y, width, height, color, image);
    nk::d3d12::processImage(canvas, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float frameX,
                           float frameY, float frameWidth, float frameHeight,
                           uint32_t color, NkImage* image) {
    canvas->base.drawImage(x, y, frameX, frameY, frameWidth, frameHeight, color,
                           image);
    nk::d3d12::processImage(canvas, image);
}

void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, float frameX, float frameY,
                           float frameWidth, float frameHeight, uint32_t color,
                           NkImage* image) {
    canvas->base.drawImage(x, y, width, height, frameX, frameY, frameWidth,
                           frameHeight, color, image);
    nk::d3d12::processImage(canvas, image);
}

void nk::canvas::beginFrame(NkCanvas* canvas, float r, float g, float b,
                            float a) {
    NK_PROFILER_BEGIN_EVENT_COLOR("DrawFrame",
                                  NK_PROFILER_COLOR(0xff, 0xff, 0));
    canvas->clearColor[0] = r;
    canvas->clearColor[1] = g;
    canvas->clearColor[2] = b;
    canvas->clearColor[3] = a;
    canvas->base.beginFrame(canvas);
}

void nk::canvas::beginFrame(NkCanvas* canvas, NkImage* renderTarget, float r,
                            float g, float b, float a) {
    NK_PROFILER_BEGIN_EVENT_COLOR("DrawFrame",
                                  NK_PROFILER_COLOR(0xff, 0xff, 0));
    canvas->clearColor[0] = r;
    canvas->clearColor[1] = g;
    canvas->clearColor[2] = b;
    canvas->clearColor[3] = a;
    canvas->renderTarget = renderTarget;
    canvas->base.beginFrame(canvas);
}

void nk::canvas::endFrame(NkCanvas* canvas) {
    canvas->base.endFrame(canvas);
    nk::d3d12::drawFrame(
        canvas, canvas->base.currentFrameIndex); // TODO: look into running this
                                                 // in a different thread.
    canvas->base.swapFrame(canvas);
    canvas->renderTarget = nullptr;
    NK_PROFILER_END_EVENT();
}

void nk::canvas::present(NkCanvas* canvas) {
    NK_PROFILER_SCOPED_EVENT_COLOR_ARGS("Present",
                                        NK_PROFILER_COLOR(0x00, 0xff, 0xff));

    // Check if we need to resize the canvas
    if (canvas->allowResize && internalResizeInfo.shouldResize) {

        nk::canvas_internal::waitFrameSyncPoint(
            canvas->base.gpuFrameSyncPoint[canvas->base.lastFrameIndex],
            canvas->base.gpuFrameWaitValue[canvas->base.lastFrameIndex]);

        for (uint32_t index = 0; index < NK_CANVAS_D3D12_MAX_FRAMES; ++index) {
            D3D_RELEASE(canvas->backbuffers[index]);
        }
        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        canvas->swapChain->GetDesc(&swapChainDesc);
        HRESULT result = canvas->swapChain->ResizeBuffers(
            swapChainDesc.BufferCount, internalResizeInfo.width,
            internalResizeInfo.height, swapChainDesc.BufferDesc.Format,
            swapChainDesc.Flags);
        if (result == S_OK) {
            for (uint32_t index = 0; index < swapChainDesc.BufferCount;
                 ++index) {
                ID3D12Resource* resource = nullptr;
                canvas->swapChain->GetBuffer(index, IID_PPV_ARGS(&resource));
                canvas->backbuffers[index] = resource;
                resource->SetName(L"NkCanvas::backbuffer");
            }
            canvas->base.resolution[0] = (float)internalResizeInfo.width;
            canvas->base.resolution[1] = (float)internalResizeInfo.height;
        } else {
            NK_PANIC("Error: Failed to resize swap chain");
        }
    }

    canvas->swapChain->Present(canvas->app->vsyncEnabled, 0);

    for (uint32_t imgIndex = 0;
         imgIndex <
         canvas->imagesToDestroy[canvas->base.currentFrameIndex].imageNum;
         ++imgIndex) {
        nk::d3d12::destroyImage(
            canvas->imagesToDestroy[canvas->base.currentFrameIndex]
                .images[imgIndex]);
    }
    canvas->imagesToDestroy[canvas->base.currentFrameIndex].reset();
}

float nk::canvas::viewWidth(NkCanvas* canvas) { return canvas->base.width(); }

float nk::canvas::viewHeight(NkCanvas* canvas) { return canvas->base.height(); }

NkImage* nk::canvas::createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                             uint32_t height) {
    return nk::d3d12::createRenderTargetImage(canvas, width, height);
}

NkImage* nk::canvas::createImage(NkCanvas* canvas, uint32_t width,
                                 uint32_t height, const void* pixels,
                                 NkImageFormat format) {
    return nk::d3d12::createImage(canvas, width, height, format, pixels);
}

bool nk::canvas::destroyImage(NkCanvas* canvas, NkImage* image) {
    if (image) {
        canvas->imagesToDestroy[canvas->base.currentFrameIndex].add(image);
        return true;
    }
    return false;
}

float nk::img::width(NkImage* image) { return image->width; }

float nk::img::height(NkImage* image) { return image->height; }

void nk::d3d12::drawFrame(NkCanvas* canvas, uint64_t currentFrameIndex) {
    NK_PROFILER_SCOPED_EVENT_COLOR_ARGS("D3D12_DrawFrame %u",
                                        NK_PROFILER_COLOR(0xff, 0x00, 0xff),
                                        currentFrameIndex);
    // Start frame
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    ID3D12Resource* backbuffer = nullptr;
    float viewWidth = (float)canvas->app->windowWidth;
    float viewHeight = (float)canvas->app->windowHeight;

    if (canvas->renderTarget) {
        viewWidth = canvas->renderTarget->width;
        viewHeight = canvas->renderTarget->height;
        backbuffer = canvas->renderTarget->buffer.resource.resource;
    } else {
        backbuffer =
            canvas->backbuffers[canvas->swapChain->GetCurrentBackBufferIndex()];
    }
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        canvas->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    nk::d3d12::instance()->device->CreateRenderTargetView(backbuffer, &rtvDesc,
                                                          rtvHandle);

    ID3D12GraphicsCommandList* commandList =
        canvas->commandLists[currentFrameIndex].commandList;
    ID3D12CommandAllocator* commandAllocator =
        canvas->commandLists[currentFrameIndex].commandAllocator;
    D3D_ASSERT(commandAllocator->Reset(),
               "Error: Failed to reset command allocator");
    D3D_ASSERT(commandList->Reset(commandAllocator, canvas->spritePSO),
               "Error: Failed to reset command list");
    NkD3D12DescriptorAllocator& descriptorAllocator =
        canvas->descriptorAllocators[currentFrameIndex];
    descriptorAllocator.reset();

    // Upload index buffer
    if (!canvas->base.indicesUploaded) {
        NkD3D12DynamicBuffer* indexBuffer =
            (NkD3D12DynamicBuffer*)canvas->base.gpuIndexBuffer;
        void* mappedIndices = nullptr;
        D3D_ASSERT(indexBuffer->uploadBuffer->Map(0, nullptr, &mappedIndices),
                   "Error: Failed to map index buffer uploead buffer");
        if (mappedIndices) {
            memcpy(mappedIndices, canvas->base.indexBufferData(),
                   NK_CANVAS_MAX_INDICES_BYTE_SIZE);
        } else {
            NK_PANIC("Error: Invalid address for index buffer");
        }
        indexBuffer->uploadBuffer->Unmap(0, nullptr);
        D3D12_RESOURCE_BARRIER indexBufferBarrier[1] = {};
        indexBufferBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        indexBufferBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        indexBufferBarrier[0].Transition.Subresource = 0;
        indexBufferBarrier[0].Transition.pResource =
            indexBuffer->resource.resource;
        indexBufferBarrier[0].Transition.StateBefore =
            indexBuffer->resource.state;
        indexBufferBarrier[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;

        commandList->ResourceBarrier(1, indexBufferBarrier);
        commandList->CopyResource(indexBuffer->resource.resource,
                                  indexBuffer->uploadBuffer);
        indexBufferBarrier[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;
        indexBufferBarrier[0].Transition.StateAfter =
            indexBuffer->resource.state;
        commandList->ResourceBarrier(1, indexBufferBarrier);
        canvas->base.indicesUploaded = true;
    }

    // Upload textures
    ID3D12Device* device = nk::d3d12::instance()->device;
    NkImageArray& imageUpload = canvas->imageUploads[currentFrameIndex];
    NkImage** images = imageUpload.images;

    for (uint32_t index = 0, num = imageUpload.imageNum; index < num; ++index) {
        NkImage* image = images[index];
        NK_ASSERT(image->cpuData != nullptr, "Error: Image CPU data is null.")

        ID3D12Resource* texture = image->buffer.resource.resource;
        ID3D12Resource* uploadBuffer = image->buffer.uploadBuffer;
        D3D12_RESOURCE_DESC desc = image->buffer.resource.desc;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
        uint32_t numRows = 0;
        uint64_t rowSizeInBytes = 0, totalBytes = 0;
        device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, &numRows,
                                      &rowSizeInBytes, &totalBytes);
        void* mapped = nullptr;
        uploadBuffer->Map(0, nullptr, &mapped);
        size_t pixelSize = nk::d3d12::getDXGIFormatBytes(desc.Format);

        for (uint32_t index = 0; index < numRows * layout.Footprint.Depth;
             ++index) {
            void* dstAddr = nk::utils::offsetPtr(
                mapped, (intptr_t)(index * layout.Footprint.RowPitch));
            const void* srcAddr = nk::utils::offsetPtr(
                (void*)image->cpuData,
                (intptr_t)(index * (desc.Width * pixelSize)));
            memcpy(dstAddr, srcAddr, (size_t)rowSizeInBytes);
        }
        uploadBuffer->Unmap(0, nullptr);

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.pResource = uploadBuffer;
        srcLocation.PlacedFootprint = layout;
        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.pResource = texture;
        dstLocation.SubresourceIndex = 0;

        D3D12_RESOURCE_BARRIER textureBufferBarrier[1] = {};
        textureBufferBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        textureBufferBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        textureBufferBarrier[0].Transition.Subresource = 0;
        textureBufferBarrier[0].Transition.pResource = texture;
        textureBufferBarrier[0].Transition.StateBefore =
            image->buffer.resource.state;
        textureBufferBarrier[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;

        commandList->ResourceBarrier(1, textureBufferBarrier);
        commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation,
                                       nullptr);

        textureBufferBarrier[0].Transition.StateAfter =
            image->buffer.resource.state;
        textureBufferBarrier[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;
        commandList->ResourceBarrier(1, textureBufferBarrier);

        nk::utils::memFree(image->cpuData);
        image->cpuData = nullptr;
        image->state |= NK_IMAGE_BIT_UPLOADED;
    }

    imageUpload.reset();

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    nk::d3d12::updateTextureAtlas(canvas->base.frameTextureAtlas, commandList);
#endif

    // Upload vertex buffers
    NkCanvasDrawBatchInternalArray& drawBatchArray =
        canvas->base.drawBatchArray[currentFrameIndex];
    D3D12_RESOURCE_BARRIER* resourceBarriers =
        (D3D12_RESOURCE_BARRIER*)_malloca(sizeof(D3D12_RESOURCE_BARRIER) *
                                          (drawBatchArray.drawBatchNum + 1));
    if (!resourceBarriers) {
        NK_PANIC("Can't allocate vertex buffer barriers");
        return;
    }
    for (uint32_t index = 0; index < drawBatchArray.drawBatchNum; ++index) {
        NkCanvasDrawBatchInternal& drawBatch =
            drawBatchArray.drawBatches[index];
        NkCanvasVertexBuffer* vertexBuffer = drawBatch.buffer;
        NkD3D12DynamicBuffer* vertexBufferDynamic =
            (NkD3D12DynamicBuffer*)vertexBuffer->gpuVertexBuffer;
        void* mappedVertices = nullptr;
        D3D_ASSERT(
            vertexBufferDynamic->uploadBuffer->Map(0, nullptr, &mappedVertices),
            "Error: Failed to map vertex buffer");
        if (mappedVertices) {
            memcpy(mappedVertices, vertexBuffer->vertices,
                   vertexBuffer->vertexCount * sizeof(NkCanvasVertex));
        } else {
            NK_PANIC("Error: Invalid address for vertex buffer");
        }
        D3D12_RANGE writeRange = {0, vertexBuffer->vertexCount *
                                         sizeof(NkCanvasVertex)};
        vertexBufferDynamic->uploadBuffer->Unmap(0, &writeRange);
        resourceBarriers[index].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarriers[index].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resourceBarriers[index].Transition.Subresource = 0;
        resourceBarriers[index].Transition.pResource =
            vertexBufferDynamic->resource.resource;
        resourceBarriers[index].Transition.StateBefore =
            vertexBufferDynamic->resource.state;
        resourceBarriers[index].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;
    }

    // batch the RTV barrier with the vertex buffers
    resourceBarriers[drawBatchArray.drawBatchNum].Type =
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarriers[drawBatchArray.drawBatchNum].Transition.pResource =
        backbuffer;
    resourceBarriers[drawBatchArray.drawBatchNum].Transition.StateBefore =
        D3D12_RESOURCE_STATE_PRESENT;
    resourceBarriers[drawBatchArray.drawBatchNum].Transition.StateAfter =
        D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarriers[drawBatchArray.drawBatchNum].Transition.Subresource = 0;
    resourceBarriers[drawBatchArray.drawBatchNum].Flags =
        D3D12_RESOURCE_BARRIER_FLAG_NONE;
    if (canvas->renderTarget) {
        resourceBarriers[drawBatchArray.drawBatchNum].Transition.StateBefore =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    commandList->ResourceBarrier(drawBatchArray.drawBatchNum + 1,
                                 resourceBarriers);
    for (uint32_t index = 0; index < drawBatchArray.drawBatchNum; ++index) {
        NkCanvasDrawBatchInternal& drawBatch =
            drawBatchArray.drawBatches[index];
        NkCanvasVertexBuffer* vertexBuffer = drawBatch.buffer;
        NkD3D12DynamicBuffer* vertexBufferDynamic =
            (NkD3D12DynamicBuffer*)vertexBuffer->gpuVertexBuffer;
        commandList->CopyResource(vertexBufferDynamic->resource.resource,
                                  vertexBufferDynamic->uploadBuffer);
    }

    for (uint32_t index = 0; index < drawBatchArray.drawBatchNum; ++index) {
        D3D12_RESOURCE_STATES before =
            resourceBarriers[index].Transition.StateBefore;
        resourceBarriers[index].Transition.StateBefore =
            resourceBarriers[index].Transition.StateAfter;
        resourceBarriers[index].Transition.StateAfter = before;
    }

    commandList->ResourceBarrier(drawBatchArray.drawBatchNum, resourceBarriers);

    // Write frame setup commands
    commandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
    if (canvas->clearColor[3] > 0.0f) {
        commandList->ClearRenderTargetView(
            canvas->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            canvas->clearColor, 0, nullptr);
    }
    commandList->SetGraphicsRootSignature(canvas->spriteRootSignature);
    commandList->SetDescriptorHeaps(
        1, &canvas->descriptorAllocators[currentFrameIndex].descriptorHeap);
    float viewDimension[] = {viewWidth, viewHeight};
    commandList->SetGraphicsRoot32BitConstants(0, 2, viewDimension, 0);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (float)viewWidth;
    viewport.Height = (float)viewHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = (uint32_t)viewWidth;
    scissor.bottom = (uint32_t)viewHeight;
    commandList->RSSetScissorRects(1, &scissor);

    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    indexBufferView.BufferLocation =
        ((NkD3D12DynamicBuffer*)canvas->base.gpuIndexBuffer)
            ->resource.resource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes =
        (uint32_t)((NkD3D12DynamicBuffer*)canvas->base.gpuIndexBuffer)
            ->resource.desc.Width;
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    commandList->IASetIndexBuffer(&indexBufferView);

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    {
        NkD3D12DescriptorTable descriptorTable =
            descriptorAllocator.allocate(1);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        device->CreateShaderResourceView(
            (ID3D12Resource*)canvas->base.frameTextureAtlas.gpuTexture,
            &srvDesc, descriptorTable.cpuHandle(0));
        commandList->SetGraphicsRootDescriptorTable(
            1, descriptorTable.gpuHandle(0));
    }
#endif

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    {
        for (uint32_t index = 0; index < canvas->imageNum; ++index) {
        }
        NkD3D12DescriptorTable descriptorTable =
            descriptorAllocator.allocate(canvas->imageNum);
        for (uint32_t index = 0; index < canvas->imageNum; ++index) {
            NkImage* image = canvas->images[index];
            image->state &= ~NK_IMAGE_BIT_BOUND_TO_DESCRIPTOR_TABLE;
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping =
                D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            device->CreateShaderResourceView(image->buffer.resource.resource,
                                             &srvDesc,
                                             descriptorTable.cpuHandle(index));
        }
        commandList->SetGraphicsRootDescriptorTable(
            1, descriptorTable.gpuHandle(0));
    }
#endif

    // Render draw batches
    NkCanvasDrawBatchInternal* drawBatches = drawBatchArray.drawBatches;
    ID3D12Resource* vertexBuffer = nullptr;

    for (uint32_t index = 0, num = drawBatchArray.drawBatchNum; index < num;
         ++index) {
        NkCanvasDrawBatchInternal& drawBatch = drawBatches[index];
        NkD3D12DynamicBuffer* drawBatchVertexBuffer =
            (NkD3D12DynamicBuffer*)drawBatch.buffer->gpuVertexBuffer;
        if (vertexBuffer != drawBatchVertexBuffer->resource.resource) {
            // vertexBuffer = drawBatchVertexBuffer->uploadBuffer;
            vertexBuffer = drawBatchVertexBuffer->resource.resource;

            D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
            vertexBufferView.BufferLocation =
                vertexBuffer->GetGPUVirtualAddress();
            vertexBufferView.SizeInBytes =
                (uint32_t)drawBatchVertexBuffer->resource.desc.Width;
            vertexBufferView.StrideInBytes = sizeof(NkCanvasVertex);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        }
#if !NK_CANVAS_TEXTURE_ATLAS_ENABLED && !NK_CANVAS_BINDLESS_RESOURCE_ENABLED
        NkD3D12Resource& textureResource = drawBatch.image->buffer.resource;
        NkD3D12DescriptorTable descriptorTable =
            descriptorAllocator.allocate(1);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureResource.desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        device->CreateShaderResourceView(textureResource.resource, &srvDesc,
                                         descriptorTable.cpuHandle(0));
        commandList->SetGraphicsRootDescriptorTable(
            1, descriptorTable.gpuHandle(0));
#endif
        commandList->DrawIndexedInstanced(drawBatch.count, 1, 0,
                                          drawBatch.bufferOffset, 0);
    }

    // End frame
    // only handle RTV resource barrier
    resourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarriers[0].Transition.pResource = backbuffer;
    resourceBarriers[0].Transition.StateBefore =
        D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    resourceBarriers[0].Transition.Subresource = 0;
    resourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    if (canvas->renderTarget) {
        resourceBarriers[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    commandList->ResourceBarrier(1, resourceBarriers);
    ID3D12CommandList* commandLists[] = {commandList};
    D3D_ASSERT(commandList->Close(), "Error: Failed to close command list");
    canvas->commandQueue->ExecuteCommandLists(1, commandLists);

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    canvas->imageNum = 0;
#endif
}

void nk::d3d12::processImage(NkCanvas* canvas, NkImage* image) {
    if (image->state == 0) {
        canvas->imageUploads[canvas->base.currentFrameIndex].add(image);
        image->state |= NK_IMAGE_BIT_SAVED;
    }
}

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
void nk::d3d12::updateTextureAtlas(NkTextureAtlas& textureAtlas,
                                   ID3D12GraphicsCommandList* commandList) {
    D3D12_TEXTURE_COPY_LOCATION textureAtlasDest{};
    textureAtlasDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    textureAtlasDest.pResource = (ID3D12Resource*)textureAtlas.gpuTexture;
    textureAtlasDest.SubresourceIndex = 0;

        D3D12_RESOURCE_BARRIER* barriers = (D3D12_RESOURCE_BARRIER*)nk::utils::memRealloc(nullptr, (sizeof(D3D12_RESOURCE_BARRIER) * ((uint32_t)textureAtlas.images.size() + 1));
	if (!barriers) {
        NK_ASSERT(0, "Error: Failed to allocated resource barriers");
        return;
	}

	uint32_t numBarriers = 0;
	{
        D3D12_RESOURCE_BARRIER& barrier = barriers[numBarriers++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = (ID3D12Resource*)textureAtlas.gpuTexture;
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	for (auto& entry : textureAtlas.images) {
        D3D12_RESOURCE_BARRIER& barrier = barriers[numBarriers++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = entry.first->buffer.resource.resource;
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = entry.first->buffer.resource.state;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	}

	commandList->ResourceBarrier(numBarriers, barriers);

	for (auto& entry : textureAtlas.images) {
        NkImage* image = entry.first;
        // if ((image->state & NK_IMAGE_BIT_TEXTURE_ATLAS_RESIDENT) > 0)
        // continue;
        NkTextureAtlasRect& rect = entry.second;
        D3D12_TEXTURE_COPY_LOCATION imageDest{};
        imageDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        imageDest.pResource = image->buffer.resource.resource;
        imageDest.SubresourceIndex = 0;

        commandList->CopyTextureRegion(&textureAtlasDest, rect.x, rect.y, 0,
                                       &imageDest, nullptr);
        // image->state |= NK_IMAGE_BIT_TEXTURE_ATLAS_RESIDENT;
        image->state &= ~NK_IMAGE_BIT_TEXTURE_ATLAS;
	}

	for (uint32_t index = 0; index < numBarriers; ++index) {
        D3D12_RESOURCE_STATES afterState =
            barriers[index].Transition.StateAfter;
        barriers[index].Transition.StateAfter =
            barriers[index].Transition.StateBefore;
        barriers[index].Transition.StateBefore = afterState;
	}

	commandList->ResourceBarrier(numBarriers, barriers);

	nk::utils::memFree(barriers);
}
#endif

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
uint32_t nk::canvas_internal::addImageToTable(NkCanvas* canvas,
                                              NkImage* image) {
    if ((image->state & NK_IMAGE_BIT_BOUND_TO_DESCRIPTOR_TABLE) > 0) {
        return image->textureId;
    }
    uint32_t textureId = canvas->imageNum++;
    canvas->images[textureId] = image;
    image->state |= NK_IMAGE_BIT_BOUND_TO_DESCRIPTOR_TABLE;
    image->textureId = textureId;
    return textureId;
}
#endif
