# Minecraft Clone (Working Title)
Yeah, I haven’t named it yet. Suggestions welcome :)
## Introduction
This is a small game project built using modern OpenGL and C++20. Think Minecraft-style voxel fun, but from scratch and fully under your control. The project is in a very early, experimental stage.

## Features
- Procedural terrain generation through chunking system
- Block breaking/placing ability
- Multiple modes
- Skin rendering system(broken)
- Multiple block types
- Collision system with AABBs
### Some screenshots
![Screenshot 2025-07-03 231412](https://github.com/user-attachments/assets/7c71a15f-02c1-4c36-b6ec-c6c13c0c7fb2)![Screenshot 2025-07-03 231457](https://github.com/user-attachments/assets/35c74a69-89f6-48f6-a708-d0e05c6931ba)

## Dependencies
To build this project, you’ll need:
- [CMake](https://cmake.org/download/)(>= 3.15)
- [Ninja](https://github.com/ninja-build/ninja)
- [A C++ compiler](https://isocpp.org/get-started)

Make sure both are installed and added to your system's PATH if you're on Windows.
##  Building the Project
### Clone the repo
Open a terminal in whatever folder and run:
```
git clone https://github.com/MAKMART/minecraft_clone.git
cd minecraft_clone
```
Choose the configuration type depending on your goal.
### Debug build:
```
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug .
ninja -C build
```
### Release build:
```
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release .
ninja -C build
```
All the build files are stored in the build directory --- duhh ---

The final binary will be located in build/bin/debug/ or build/bin/release/ depending on your build type.
## Troubleshooting
If you run into any build issues or have questions, feel free to email me:
martinmarco813@gmail.com 
