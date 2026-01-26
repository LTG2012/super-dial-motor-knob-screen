# VSCode LVGL Simulator

This folder contains a minimal PC simulator for the UI using LVGL + SDL2.

## Prereqs (Windows)

Option A: vcpkg
1. Install SDL2:
   - `vcpkg install sdl2`
2. Configure with the vcpkg toolchain:
   - `cmake -S pc_sim -B pc_sim/build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake`

Option B: MSYS2
1. Install SDL2:
   - `pacman -S mingw-w64-x86_64-SDL2`
2. Configure normally:
   - `cmake -S pc_sim -B pc_sim/build`

## Build & Run

From the repo root:
```
cmake -S pc_sim -B pc_sim/build
cmake --build pc_sim/build
pc_sim/build/superdial_ui_sim.exe
```

Close the window to exit.
