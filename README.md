> [!CAUTION]
> This repository does not include, distribute, or provide access to any game assets, executables, or copyrighted content. It only contains the source code to build the recompilation. **You must own a legitimate copy of Condemned 2: Bloodshot** and extract the required assets from it.

# Condemned2Recomp
This is a static recompilation of **Condemned 2: Bloodshot (Xbox 360)** for Native PC built through RexGlue-SDK. 

## Installation
* Using [extract-xiso](https://github.com/XboxDev/extract-xiso) or [xdvdfs](https://github.com/antangelo/xdvdfs), extract all game files from your legally owned copy of **Condemned 2: Bloodshot**. 
* Place extracted game files into the *"Assets"* folder.
* Run *"condemned2recomp.exe"* to start **Condemned 2: Bloodshot**.

## Controls

Keyboard & mouse support is still work-in-progress! Best played with a gamepad.

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
