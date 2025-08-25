# Minecraft Clone (Working Title) 🎮

Yeah, I haven’t named it yet. Suggestions welcome 😅

## Introduction 📝

This is a small game project built using modern OpenGL and C++20. Think Minecraft-style voxel fun, but from scratch and fully under your control. The project is in a very early, experimental stage 🚀.

Since this is meant for me to learn programming, don't expect high-quality stuff here for some time 😜.

## Features ✨

* Procedural terrain generation through chunking system 🌄
* Block breaking/placing ability ⛏️
* Multiple modes 🎯
* Skin rendering system (broken) 🧑‍🎨
* Multiple block types 🧱
* Collision system with AABBs ⚔️

### Some screenshots 🖼️

![Screenshot 2025-07-03 231412](https://github.com/user-attachments/assets/7c71a15f-02c1-4c36-b6ec-c6c13c0c7fb2)
![Screenshot 2025-07-03 231457](https://github.com/user-attachments/assets/35c74a69-89f6-48f6-a708-d0e05c6931ba)

## Dependencies 🔧

To build this project, you’ll need:

* [CMake](https://cmake.org/download/) `(>= 3.30)` 🏗️
* [Ninja](https://github.com/ninja-build/ninja) ⚡
* [A C++ compiler](https://isocpp.org/get-started) 🖥️

Make sure all three are installed and added to your system's `PATH` if you're on Windows.

## Building the Project from Source 🛠️

After building, all build files will be stored in the `build` directory.
The final binary will be located in:

* `build/bin/debug/` for **Debug** builds 🐛
* `build/bin/release/` for **Release** builds 🚀

---

### 1. Clone the Repository 📥

Open a terminal in your desired folder and run:

```bash
git clone https://github.com/MAKMART/minecraft_clone.git
cd minecraft_clone
```
> ⚠️ Warning: You need to have [git](https://git-scm.com/downloads) installed and added to your `PATH` if you're on Windows.
---

### 2. Configure the Project ⚙️

Choose the configuration type:

```bash
# For Debug build
cmake --preset debug

# For Release build
cmake --preset release
```

> ⚡ Tip: Pick one preset depending on whether you want a Debug or Release build.

---

### 3. Build the Project 🏗️

Use the same preset you chose for configuration:

```bash
cmake --build --preset <preset>
```

For example:

```bash
# Debug build
cmake --build --preset debug

# Release build
cmake --build --preset release
```

> ✅ Once configured, you only need to run the build command again if you make changes.

## Troubleshooting 🛡️

If you run into any build issues or have questions, feel free to email me:
✉️ [martinmarco813@gmail.com](mailto:martinmarco813@gmail.com
