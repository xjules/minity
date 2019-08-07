#!/usr/bin/bash

echo "Build started."

export PROJECT_HOME=`pwd`
echo "PROJECT_HOME=$PROJECT_HOME"

cd ext

for name in  glfw #glm glbinding globjects
do
    echo "Configuring $name:"
    cd $PROJECT_HOME/ext/$name
    [ -d ./build ] && rm -rf build
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH="$PROJECT_HOME/lib" -DCMAKE_PREFIX_PATH="$PROJECT_HOME/lib" -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
    echo "Building $name"
    make install
    echo "Done."
done

for name in  glm #glbinding globjects
do
    echo "Configuring $name:"
    cd $PROJECT_HOME/ext/$name
    [ -d ./build ] && rm -rf build
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH="$PROJECT_HOME/lib" -DCMAKE_PREFIX_PATH="$PROJECT_HOME/lib" -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DGLM_TEST_ENABLE=OFF 
    echo "Building $name"
    make install
    echo "Done."
done

for name in  glbinding #globjects
do
    echo "Configuring $name:"
    cd $PROJECT_HOME/ext/$name
    [ -d ./build ] && rm -rf build
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH="$PROJECT_HOME/lib" -DCMAKE_PREFIX_PATH="$PROJECT_HOME/lib" -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DOPTION_BUILD_EXAMPLES=OFF -Dglfw3_DIR=$PROJECT_HOME/lib/lib64/cmake/glfw3 -DOPTION_BUILD_TOOLS=ON
    echo "Building $name"
    make install
    echo "Done."
done

for name in globjects
do
    echo "Configuring $name:"
    cd $PROJECT_HOME/ext/$name
    [ -d ./build ] && rm -rf build
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH="$PROJECT_HOME/lib" -DCMAKE_PREFIX_PATH="$PROJECT_HOME/lib" -DBUILD_SHARED_LIBS=OFF -DOPTION_BUILD_EXAMPLES=OFF 
    echo "Building $name"
    make install
    echo "Done."
done


echo "Removing installation files."
cd $PROJECT_HOME
rm -rf ext
echo "Done."

