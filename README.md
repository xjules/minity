# minity - A minimal graphics engine


## Getting Started

Minity was developed under Windows using Microsoft Visual Studio 2019. It uses [CMake](https://cmake.org/) as its build system and does not contain anything else platform-specific, so it should in principle work using other compilers and operating systems, but this has not been tested. Should there be any issues, we are grateful for any input that helps us make the software work on as many platforms as possible.

## Prerequisites

The project uses [CMake](https://cmake.org/) and relies on the following libraries: 

- [GLFW](https://www.glfw.org/) 3.3 or higher (https://github.com/glfw/glfw.git) for windowing and input support
- [glm](https://glm.g-truc.net/) 0.9.9.5 or higher (https://github.com/g-truc/glm.git) for its math funtionality
- [glbinding](https://github.com/cginternals/glbinding) 3.1.0 or higher (https://github.com/cginternals/glbinding.git) for OpenGL API binding
- [globjects](https://github.com/cginternals/globjects) 2.0.0 or higher (https://github.com/cginternals/globjects.git) for additional OpenGL wrapping
- [Dear ImGui](https://github.com/ocornut/imgui) 1.71 or higher (https://github.com/ocornut/imgui.git) for GUI elements
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) 3.3.9 or higher (https://git.code.sf.net/p/tinyfiledialogs/code) for dialog functionality
- [stb](https://github.com/nothings/stb/) (stb_image.h 2.22 or higher and stb_image_write.h 1.13 or higher) or higher (https://github.com/nothings/stb.git) for image loading and saving

The project uses vcpkg (https://vcpkg.io) for dependency management, so this should take care of everything. There is a manifest file called ```vcpkg.json``` in the project root folder. When building with CMake for the first time, all dependencies should be downloaded and installed automatically.

## Building

If you are using Visual Studio, you can use its integrated CMake support to build and run the project.

When instead building from the command line, run the following commands from the project root folder:

```
mkdir build
cd build
cmake ..
```

After this, you can compile the debug or release versions of the project using 

```
cmake --build --config Debug
```

and/or

```
cmake --build --config Release
```

After building, the executables will be available in the ```./bin``` folder.

## Running

To correctly locate shaders and other resources (which are stored in the  ```./res``` folder), the program requires the current working directory to be the project root folder. A launch configuration file for Visual Studio (```./.vs/launch.vs.json```) that takes care of this is included, just select ```minity``` as a startup item in the toolbar. When running from the command line, make sure that you are in the project root folder and execute

```
./bin/minity
```

## Usage

After starting the program, a file dialog will pop up and ask you for a Wavefront OBJ File file. Some basic usage instructions are displayed in the console window.

