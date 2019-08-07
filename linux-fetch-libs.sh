#!/usr/bin/bash

echo "Downloading external dependencies..."

[ -d ./ext ] && rm -rf ./ext

mkdir ext
cd ext

git clone https://github.com/glfw/glfw.git
git clone https://github.com/g-truc/glm.git
git clone https://github.com/cginternals/glbinding.git
git clone https://github.com/cginternals/globjects.git

cd ..

[ ! -d ./lib ] && mkdir lib
cd lib

[ -d ./imgui ] && rm -rf imgui
git clone https://github.com/ocornut/imgui.git

[ -d ./tinyfd ] && rm -rf tinyfd
git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd

[ -d ./stb ] && rm -rf stb
git clone https://github.com/nothings/stb.git

cd ..

echo "Done."