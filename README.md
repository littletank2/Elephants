# Elephants

A small C++20/SFML 3 playable prototype about an elephant herd changing a hex-map ecosystem.


## Documentation

- [Codebase documentation](docs/CODEBASE.md)
- [Class diagram](docs/CLASS_DIAGRAM.md)
## Build on Windows

The project uses vcpkg manifest mode. The included `windows-vcpkg` preset expects vcpkg at `C:/vcpkg`, which matches this machine.

```powershell
cmake --preset windows-vcpkg
cmake --build --preset windows-debug
.\build\windows-vcpkg\Debug\elephants.exe
```

If you use a different vcpkg location, either set `VCPKG_ROOT` and use `ninja-vcpkg`, or edit `CMakePresets.json`.

## Controls

- `Q`, `W`, `E`, `A`, `S`, `D`: choose one of the six hex directions.
- Hold `Shift`: move fast without eating grass.
- `F`: toggle fast movement on/off.
- `Space`: pause/resume.
- `R`: regenerate the world.
- `Esc`: quit.

## Goal

Keep the herd fed, grow it, and reduce forest coverage below 5% of passable land. Large herds affect a wider front and have a better chance to turn forest into grassland.

## Tests

```powershell
cmake --build --preset windows-debug
ctest --test-dir .\build\windows-vcpkg -C Debug --output-on-failure
```