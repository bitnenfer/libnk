import os
import shutil

if os.path.exists("release"):
    shutil.rmtree("release")

os.mkdir("release")
os.mkdir("release/windows-x64")
os.mkdir("release/windows-x64/lib")
os.mkdir("release/webassembly")
os.mkdir("release/webassembly/lib")

shutil.copytree("include", "release/windows-x64/include")
shutil.copytree("include", "release/webassembly/include")

shutil.copyfile("out/build/x64-release/nk/nk.lib", "release/windows-x64/lib/nk.lib")
shutil.copyfile("out/build/web-wasm-release/nk/libnk.a", "release/webassembly/lib/libnk.a")

version_name = input("Version name: ")

shutil.make_archive("release/nk-windows-x64-" + version_name, "zip", "release/windows-x64/")
shutil.make_archive("release/nk-webassembly-" + version_name, "zip", "release/webassembly/")
