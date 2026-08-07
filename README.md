> [!IMPORTANT]
> This repository does not include, distribute, or provide access to any game assets. It only contains the source code to build the recompilation. **You must own a legitimate copy of Condemned 2: Bloodshot** and extract the required assets from it.

> [!CAUTION]
> This project is a work-in-progress, and while it is in playable state, you might experience bugs and crashes!

# Condemned2Recomp

This is a static recompilation of **Condemned 2: Bloodshot (Xbox 360)** for Native PC built through RexGlue-SDK.

<div align="center"><a href="https://www.youtube.com/watch?v=028_cJIhIjA"><img src="https://img.youtube.com/vi/028_cJIhIjA/maxresdefault.jpg" alt="Condemned 2 Recomp ( Preview for v0.1.1 - Performance Improvements" width="75%"></a><br>(Click thumbnail above to watch on Youtube)</div>

## Installation
* Run *"condemned2recomp.exe"*. On first launch it asks for the disc image (*.iso*) dumped from your legally owned copy of **Condemned 2: Bloodshot** and extracts the game files into the *"Assets"* folder for you (~7 GiB of free space needed).
* **(Optional)** Configure your graphic settings by modifying *"condemned2recomp.toml"*.

<details>
<summary>Manual installation (alternative)</summary>

* Using [extract-xiso](https://github.com/XboxDev/extract-xiso) or [xdvdfs](https://github.com/antangelo/xdvdfs), extract all game files from your legally owned copy of **Condemned 2: Bloodshot**.
* In the same folder as *"condemned2recomp.exe"*, create a new folder named *"Assets"*.
* Place extracted game files into the *"Assets"* folder.
* For an unattended/headless install, set the environment variable `CONDEMNED2_INSTALL_ISO` to the path of your disc image before launching.
</details>

## Controls

Keyboard & mouse support is still work-in-progress! Currently, it is recommended to use a gamepad to play.

| Action | Key |
| --- | --- |
| Movement | `W` `A` `S` `D` |
| Use | `E` |
| Swing Left / Aim | `LMB` |
| Swing Right / Shoot | `RMB` |
| Kick | `Space` |
| Reload | `R` |
| Sprint | `Shift` |
| Flashlight | `F` |
| Check | `H` |
| Pause | `Esc` |
| Settings | `F4` |
| Console | `` ` `` |

## Configure & Build

### Prerequisites

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk)
- [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/vs/community) **with the Desktop development with C++ workload**
- CMake 3.25+
- LLVM/Clang 20+
- Ninja

### Build
#### Download
```
git clone --recursive https://github.com/psxrestore/Condemned2Recomp.git
```
#### Windows
```
cd Condemned2Recomp
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release --target condemned2recomp_codegen
cmake --build --preset win-amd64-release
```
#### Linux **(Untested)**
```
cd Condemned2Recomp
cmake --preset linux-amd64-release
cmake --build --preset linux-amd64-release --target condemned2recomp_codegen
cmake --build --preset linux-amd64-release
```

## Credits
* [RexGlue-SDK](https://github.com/rexglue/rexglue-sdk)
* [Ghidra](https://github.com/NationalSecurityAgency/ghidra)
* [RenderDoc](https://github.com/baldurk/renderdoc)
