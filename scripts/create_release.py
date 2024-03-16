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
os.mkdir("release/webassembly/lib/release")
os.mkdir("release/webassembly/lib/debug")

shutil.copytree("include", "release/windows-x64/include")
shutil.copytree("include", "release/webassembly/include")

shutil.copyfile("out/build/x64-release/nk/nk.lib", "release/windows-x64/lib/release/nk.lib")
shutil.copyfile("out/build/web-wasm-release/nk/libnk.a", "release/webassembly/lib/release/libnk.a")

shutil.copyfile("out/build/x64-debug/nk/nk.lib", "release/windows-x64/lib/debug/nk.lib")
shutil.copyfile("out/build/web-wasm-debug/nk/libnk.a", "release/webassembly/lib/debug/libnk.a")

version_name = input("Version name: ")

shutil.make_archive("release/nk-" + version_name + "-windows-x64", "zip", "release/windows-x64/")
shutil.make_archive("release/nk-" + version_name + "-webassembly", "zip", "release/webassembly/")
