import os
import shutil

if os.path.exists("release"):
    shutil.rmtree("release")

os.mkdir("release")
os.mkdir("release/windows-x64")
os.mkdir("release/windows-x64/lib")
os.mkdir("release/windows-x64/lib/release")
os.mkdir("release/windows-x64/lib/debug")
os.mkdir("release/webassembly")
os.mkdir("release/webassembly/lib")
os.mkdir("release/webassembly/lib/webgpu")
os.mkdir("release/webassembly/lib/webgpu/release")
os.mkdir("release/webassembly/lib/webgpu/debug")
os.mkdir("release/webassembly/lib/webgl")
os.mkdir("release/webassembly/lib/webgl/release")
os.mkdir("release/webassembly/lib/webgl/debug")

shutil.copytree("include", "release/windows-x64/include")
shutil.copytree("include", "release/webassembly/include")

shutil.copyfile("out/build/x64-release/nk/nk.lib", "release/windows-x64/lib/release/nk.lib")
shutil.copyfile("out/build/web-wasm-webgpu-release/nk/libnk.a", "release/webassembly/lib/webgpu/release/libnk.a")
shutil.copyfile("out/build/web-wasm-webgl-release/nk/libnk.a", "release/webassembly/lib/webgl/release/libnk.a")

shutil.copyfile("out/build/x64-debug/nk/nk.lib", "release/windows-x64/lib/debug/nk.lib")
shutil.copyfile("out/build/web-wasm-webgpu-debug/nk/libnk.a", "release/webassembly/lib/webgpu/debug/libnk.a")
shutil.copyfile("out/build/web-wasm-webgl-debug/nk/libnk.a", "release/webassembly/lib/webgl/debug/libnk.a")

version_name = input("Version name: ")

shutil.make_archive("release/nk-" + version_name + "-windows-x64", "zip", "release/windows-x64/")
shutil.make_archive("release/nk-" + version_name + "-webassembly", "zip", "release/webassembly/")
