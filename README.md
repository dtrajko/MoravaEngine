# MoravaEngine

## About
2D/3D graphics engine based on C++ and OpenGL ES 2.0.
Its current purpose is experimenting with various CGI concepts and techniques:

* Phong lighting model
* Shadows (directional light, omni-directional shadows)
* Physically based rendering (PBR)
* Image based lighting (IBL)
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

## Installation
Build automation based on CMake (in progress)

### Mac OS requirements

```
brew install cmake gcc git doxygen
```

### Linux requirements:

```
sudo dnf install git cmake make libXmu-devel libXi-devel libGL-devel mesa-libGL-devel mesa-libGLU-devel libXrandr-devel libXinerama-devel libXcursor-devel doxygen
```

### Project setup:
```
$ git clone --recurse-submodules https://github.com/dtrajko/MoravaEngine.git
$ mkdir MoravaEngine/MoravaEngine/build

$ cd MoravaEngine/vendor/cross-platform/assimp/contrib/zlib
$ cmake .
$ make

$ cd MoravaEngine/vendor/cross-platform/assimp
$ cmake .
$ make

$ cd MoravaEngine/vendor/cross-platform/glfw
$ cmake .
$ make

$ cd MoravaEngine/vendor/cross-platform/bullet3
$ cmake .
$ make

$ cd MoravaEngine/MoravaEngine/build
$ cmake ..
$ cmake --build .
```

### Fetching submodules for the main project cloned without submodules:
```
git submodule update --init --recursive
```

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

## Images

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-08-12_16-53-48.png)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-07-08_0158.jpg)

![image](https://raw.githubusercontent.com/dtrajko/MoravaEngine/master/MoravaEngine/Screenshots/2020-03-18-0101.jpg)
