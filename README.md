# Minecraft Clone
## Introduction 📝

This is a small minecraft clone built using OpenGL 4.6 and C++ 23. The project is in a very early, experimental stage 🚀.

Since this is meant for me to learn programming, don't expect high-quality stuff here for some time.

### Screenshots 🖼️

WIP

## Dependencies 🔧

To build this project, you'll need:

* [CMake](https://cmake.org/download/) `(>= 4.2.3)`
* [Ninja](https://github.com/ninja-build/ninja)
* [A C++ compiler](https://isocpp.org/get-started)
* [sccache](https://github.com/mozilla/sccache)
* [mold](https://github.com/rui314/mold)

Make sure all four are installed and added to your system's `PATH` if you're on Windows.

## Building the Project from Source 🛠️

After building, all build files will be stored in the `build` directory.
The final binary will be located in:

* `build/bin/Debug/` for **Debug** builds
* `build/bin/Release/` for **Release** builds

---

### 1. Clone the Repository 📥

Open a terminal in your desired folder and run:

```bash
git clone --recurse-submodules https://github.com/MAKMART/minecraft_clone.git
cd minecraft_clone
```
> ⚠️ Warning: You need to have [git](https://git-scm.com/downloads) installed.
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

If you run into any build issues or have questions, feel free to [email me](mailto:martinmarco813@gmail.com).
