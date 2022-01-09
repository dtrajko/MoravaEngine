# MoravaEngine

## About
2D/3D graphics engine written in C++ language.

It currently supports the following graphics APIs:

- OpenGL 3.3+
- Vulkan 1.2
- DirectX 11

Its current purpose is to experiment with various CG concepts and techniques:

* Phong lighting model
* Shadows (directional light, omni-directional shadows)
* Physically based rendering (PBR)
* Image based lighting (IBL)
* Screen-space ambient occlusion (SSAO)
* Particle systems
* Instanced rendering
* Surface reflection and refraction
* Framebuffers
* Post-processing
* Level of detail
* Scene editor
* Perlin noise generator
* Procedural landmass generation
* Voxel based volumes
* Marching cubes algorithm
* Physics simulations
* Skeletal animation

## Credits
* Yan Chernikov a.k.a. [TheCherno](https://twitter.com/thecherno) [ [Hazel](https://github.com/TheCherno/Hazel) ]
* [ThinMatrix](https://twitter.com/ThinMatrix/ ) [ [ThinMatrix YouTube channel](https://www.youtube.com/user/ThinMatrix) ]
* [Ben Cook](https://www.udemy.com/user/ben-cook-19/) [ Vulkan and OpenGL courses on Udemy ]
* [Joey de Vries](https://twitter.com/JoeyDeVriez) [ [learnopengl.com](https://learnopengl.com/) ]
* [Sascha Willems](https://twitter.com/SaschaWillems2) [ [Vulkan C++ examples and demos](https://github.com/SaschaWillems/Vulkan) ]
* [Sebastian Lague](https://twitter.com/sebastianlague) [ [Sebastian Lague YouTube channel](https://www.youtube.com/c/SebastianLague) ]
* [ilya73](https://www.cgtrader.com/ilya73 ) [ [Gladiator free VR](https://www.cgtrader.com/free-3d-models/character/man/gladiator-7ac625db-88c2-46ed-a2f1-182fd1f79739) ]

<!-- Installation & setup -->

## Installation
Build automation based on CMake (in progress)

### Project setup:
```
$ git clone --recursive https://github.com/dtrajko/MoravaEngine.git
```

```
$ cd MoravaEngine
```

* Build the MoravaEngine with CMake

```
$ mkdir ./MoravaEngine/build
$ cd ./MoravaEngine/build
$ cmake ..
$ cmake --build ..
```

* Open the solution ./MoravaEngine/build/MoravaEngine.sln

* Build assimp with CMake
```
$ cd ./vendor/cross-platform/assimp
$ cmake .
```

* Install GLFW
```
$ cd ./vendor/cross-platform/glfw
$ cmake .
```

* Build shaderc - use python to run
```
$ python ./vendor/cross-platform/shaderc/utils/git-sync-deps
```

* Build shaderc with CMake
```
$ cd ./vendor/cross-platform/shaderc
$ cmake .
```

* Open solution vendor/cross-platform/shaderc/shaderc.sln and build it in Visual Studio.

```
$ cd ./vendor/DirectXTex
$ cmake .
```

* Open solution vendor/DirectXTex/DirectXTex.sln in Visual Studio and build it.

* Build yaml-cpp project in MoravaEngine solution in Visual Studio.

* Build all spirv-cross-* projects in MoravaEngine solution in Visual Studio.

* Build BulletCollision project in MoravaEngine solution in Visual Studio.
* Build LinearMath project in MoravaEngine solution in Visual Studio.
* Build BulletDynamics project in MoravaEngine solution in Visual Studio.

<!-- Help section -->

## Keyboard and Mouse shortcuts

* Add Mesh or Model: Left CTRL + Left Mouse Button  
* Camera Rotation: Right Mouse Button  
* Camera Movement: W) forward, A) left, S) back, D) right, Q) down, E) up  
* Fast Movement: Left SHIFT + W|A|S|D|Q|E  
* Enable Gizmo: TAB + Left Mouse Button  
* Toggle Gizmo Modes: 1) Translate, 2) Scale, 3) Rotate, 4) Disable  
* Scene Save: Left CTRL + S  
* Scene Reset: Left CTRL + R  
* Scene Load: Left CTRL + L  
* Toggle Wireframe Mode: R  
* Copy scene object: Left CTRL + C  

<!-- Gallery (images) section -->

## Images

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2021-09-15_04-36-54.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2021-03-23_07-35-56.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-10-14_16-14-02.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-10-14_16-23-01.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-09-21_03-46-55.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-03-19-2106.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-03-18-0101.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-08-12_16-53-48.png)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-07-08_0158.jpg)

<!--

### Mac OS requirements

```
brew install cmake gcc git doxygen
```

### Linux requirements:

```
sudo dnf install git cmake make libXmu-devel libXi-devel libGL-devel mesa-libGL-devel mesa-libGLU-devel libXrandr-devel libXinerama-devel libXcursor-devel doxygen
```

* Build the Sandbox solution with CMake:
```
$ mkdir ./Sandbox/build
$ cd ./Sandbox/build
$ cmake ..
```

* Open the Sandbox/build/Sandbox.sln in Visual Studio, build projects ExampleApp and Hazel-ScriptCore, and close the solution.


* Install assimp with cmake
```
$ cd vendor/cross-platform/assimp
$ cmake .
$ cmake --build .
```

* Build assimp project in MoravaEngine solution in Visual Studio.

* Install GLFW
```
$ cd ./vendor/cross-platform/glfw
$ cmake .
```

* Build GLFW project in MoravaEngine solution in Visual Studio.

```
$ cd ./vendor/cross-platform/assimp/contrib/zlib
$ cmake .
$ cmake --build .

$ cd ./vendor/cross-platform/bullet3
$ cmake .
$ cmake --build .

$ cd ./vendor/box2d
$ ./build.sh

$ cd ./vendor/cross-platform/SPIRV-Cross
$ cd cmake .

$ cd ./vendor/cross-platform/yaml-cpp
$ mkdir build
$ cd build
$ cmake ..
```

### Fetching submodules for the main project cloned without submodules:
```
git submodule update --init --recursive
```

* Build DirectXTex in MoravaEngine solution in Visual Studio.

* Build zlibstatic project in MoravaEngine solution in Visual Studio.

* Build IrrXML project in MoravaEngine solution in Visual Studio.

* Build box2d project in MoravaEngine solution in Visual Studio.

-->
