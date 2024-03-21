cmake_minimum_required(VERSION 3.2)

if (USE_EMSCRIPTEN)
  if (EXISTS "${CMAKE_SOURCE_DIR}/emsdk_path.txt")
    file(READ "${CMAKE_SOURCE_DIR}/emsdk_path.txt" EMSDK_PATH)
    set(CMAKE_TOOLCHAIN_FILE "${EMSDK_PATH}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
  else()
    message(FATAL_ERROR "Missing emsdk_path.txt file. Create it locally at the root of the project and write the path to the Emscripten SDK. For example: C:/emsdk/")
  endif()
endif()

project(nk LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

include_directories(
  include/
)

if (WIN32)
    add_definitions(
        -DNK_CANVAS_BINDLESS_RESOURCE_ENABLED
        -DNK_PLATFORM_WINDOWS
    )
elseif(USE_EMSCRIPTEN)
    add_definitions(
        -DNK_CANVAS_TEXTURE_ATLAS_ENABLED
        -DNK_PLATFORM_WEB
    )
endif()



add_subdirectory(src nk)
add_subdirectory(examples)
