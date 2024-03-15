project(nk LANGUAGES CXX)

if (WIN32)
    add_library(nk STATIC
    backend/windows/windows_app.cpp
    backend/windows/windows_hid.cpp
    backend/windows/windows_utils.cpp
    backend/windows/windows_canvas_d3d12.cpp
    backend/canvas_internal.cpp
    backend/hid_internal.cpp
    backend/texture_packer.cpp
    )
    if (DEFINED ENV{SUPERLUMINAL_API_DIR})
        target_include_directories(nk PRIVATE $ENV{SUPERLUMINAL_API_DIR}/include/)
        target_link_libraries(nk PRIVATE $ENV{SUPERLUMINAL_API_DIR}/lib/x64/PerformanceAPI_MD\$<\$<CONFIG:Debug>:d>.lib)
        target_compile_definitions(nk PRIVATE NK_SUPERLUMINAL_ENABLED=1)
    endif()

elseif (EMSCRIPTEN)
    add_library(nk STATIC
    backend/wasm/wasm_app.cpp
    backend/wasm/wasm_hid.cpp
    backend/wasm/wasm_utils.cpp
    backend/wasm/wasm_canvas_webgpu.cpp
    backend/canvas_internal.cpp
    backend/hid_internal.cpp
    backend/texture_packer.cpp
    )
endif()
