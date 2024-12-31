# TerrariaClone

It's a clone of the `Terraria` game in `C++` using the [LLGL](https://github.com/LukasBanana/LLGL) library.

## Build

Requirements:

* cmake 3.18+
* ninja
* [_Optional_] glslang executable in the root folder of the project to compile vulkan shaders.

### Debug build

```
cmake -S . -B ./build/ -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build/
./build/TerrariaClone.exe [options]
```

### Release build

_Debug tools don't work in release build_

```
cmake -S . -B ./build_release/ -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build ./build_release/
./build_release/TerrariaClone.exe [options]
```

### Options

`--backend <d3d11|d3d12|vulkan|opengl|metal>` Set a rendering backend (D3D11, D3D12 work only on Windows; Metal works only on macOS)

`--vsync` Enable vertical synchronization

`--pause` Don't initialize the game until any key is pressed (Useful when debugging with `RenderDoc`)

`--fullscreen` Enable fullscreen mode

## Keymappings

### General

`A` - Walk leftward

`D` - Walk rightward

`Space` - Jump

`LMB` - Use item

`Esc` - Show/Hide inventory

`Minus` - Zoom out the camera

`Equals` - Zoom in the camera

`1` - Select the first hotbar slot

`2` - Select the second hotbar slot

`3` - Select the third hotbar slot

`4` - Select the fourth hotbar slot

`5` - Select the fifth hotbar slot

`6` - Select the sixth hotbar slot

`7` - Select the seventh hotbar slot

`8` - Select the eighth hotbar slot

`9` - Select the ninth hotbar slot

`0` - Select the tenth hotbar slot

`K` - Spawn particles at the cursor position

`F10` - Hide/Show FPS (in the bottom left corner)

### Debug

`F` - Toggle free camera mode

### When in free camera mode

`W` - Move the camera upward

`S` - Move the camera downward

Hold `Shift` to move the camera faster

Hold `Alt` to move the camera slower

Click/Hold `RMB` to teleport the player to the cursor position
