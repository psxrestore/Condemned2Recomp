> [!IMPORTANT]
> This repository does not include, distribute, or provide access to any game assets. It only contains the source code to build the recompilation. **You must own a legitimate copy of Condemned 2: Bloodshot** and extract the required assets from it.

> [!CAUTION]
> This project is a work-in-progress, and while it is in playable state, you might experience bugs and crashes!

# Condemned2Recomp

This is a static recompilation of **Condemned 2: Bloodshot (Xbox 360)** for Native PC built through RexGlue-SDK.

<div align="center"><a href="https://www.youtube.com/watch?v=028_cJIhIjA"><img src="https://img.youtube.com/vi/028_cJIhIjA/maxresdefault.jpg" alt="Condemned 2 Recomp ( Preview for v0.1.1 - Performance Improvements )" width="75%"></a><br>(Click thumbnail above to watch on Youtube)</div>

## Controls

| Action | Key |
| --- | --- |
| Movement | `W` `A` `S` `D` |
| Sprint | `Shift` |
| Use | `E` |
| Kick | `Space` |
| Swing Left / Shoot | `LMB` |
| Swing Right / Aim | `RMB` |
| Throw Weapon/Item | `G` |
| Reload | `R` |
| Flashlight | `F` |
| Check | `H` |
| Pause | `Esc` |
| Settings | `F4` |
| Console | `` ` `` |

## Cheats

Open the settings menu with `F4` and go to **Condemned 2 > Cheats**:

- **Unlock FPS Mode for every mission**: FPS Mode can be started from any mission without earning it first.
- **Infinite ammo**: weapons never run out of ammo.
- **Unlock the holster in every mission**: the holster works from the first mission, without finishing Mission 4 first. While it is on, the Drop key (`B`) stows your weapon in the holster instead of dropping it; press it again to swap back. `G` still throws.
- **Give firearm / Give melee weapon**: pick a weapon to add it to your inventory (in-game only). Firearms come with 9999 reserve ammo. Weapons that the current mission doesn't use get their model, animations and sounds loaded from the mission that has them, which takes a moment and uses memory. To stay within the game's memory, at most two of these extra sets are kept loaded: spawning from a third unloads the oldest, and weapons you spawned from it can lose their model.
- **Give weapon by record name**: type a weapon record name and press Enter.

These settings are saved if you press **Save to config**, so they will still be on the next time you start the game.

## Configure & Build

### Prerequisites

- [Visual Studio 2022 or newer Community Edition](https://visualstudio.microsoft.com/vs/community) **with the Desktop development with C++ workload** (its bundled CMake and Ninja are fine)
- CMake 3.25+
- LLVM/Clang 20+
- Ninja
- [xdvdfs](https://github.com/antangelo/xdvdfs) (optional, to unpack the ISO yourself instead of using the in-game launcher)

The [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) is included as a submodule in `thirdparty/rexglue-sdk`, no separate install needed.

### Game files for codegen
Code generation recompiles the game's executable, so place these in `assets/` before building:
- `default.xex` from your game ISO
- `default.xexp` from the Title Update

### Build
#### Download
```
git clone --recursive https://github.com/psxrestore/Condemned2Recomp.git
```
(If you forgot `--recursive`, run `git submodule update --init --recursive`.)
#### Windows
Run these from an **x64 Native Tools / Developer Command Prompt** (or any shell with the MSVC environment loaded) so Clang can find the Windows SDK.
```
cd Condemned2Recomp
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release --target condemned2recomp_codegen
cmake --build --preset win-amd64-release
```
The executable ends up in `out/build/win-amd64-release/`. On first launch it asks for the ISO and downloads the Title Update, unless the game files are already in `Assets/` next to the exe.
#### Linux **(Untested/WIP)**
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
* [PicoSHA2](https://github.com/okdshin/PicoSHA2)
* [curl](https://github.com/curl/curl)