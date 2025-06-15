# Minecraft Clone (Working Title)
Yeah, I haven’t named it yet. Suggestions welcome :)
## Introduction
This is a small game project built using modern OpenGL and C++20. Think Minecraft-style voxel fun, but from scratch and fully under your control. The project is in a very early, experimental stage.
### Some screenshots
![Screenshot 2025-06-15 221429](https://github.com/user-attachments/assets/b2461d68-5f1a-4081-bac3-c21720d3101f)![Screenshot 2025-06-15 221548](https://github.com/user-attachments/assets/5b00ad8b-644f-4b32-99a5-bc10c30b1473)


## Dependencies
To build this project, you’ll need:
- [CMake](https://cmake.org/download/)(>= 3.15)
- [Ninja](https://github.com/ninja-build/ninja)

Make sure both are installed and added to your system's PATH if you're on Windows.
##  Building the Project
### Clone the repo
Open a terminal in whatever folder you want and run:
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
