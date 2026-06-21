# MateROV — Build Instructions (macOS)

This project uses:

- SDL2
- SDL2_ttf
- Boost (ASIO)
- ImGui (already included in `MateROV/external/`)

## Dependencies

Install using [Homebrew](https://brew.sh):

```bash
brew install sdl2 sdl2_ttf boost
```

## Clone Repo (need to use recursive submodules)

### No GitHub Desktop
```bash
git clone --recurse-submodules https://github.com/PCC-MateROV/ROV.git
```

### GitHub Desktop
Clone the repository with GitHub Desktop. Then open a terminal and run this command in the repo folder.

```bash
git submodule update --init --recursive
```

## Configuring CLion

You should be able to open the MateROV directory and run.

You may have to change the working directory to the root folder.

## Configuring VSCode

### Install CMake
```bash
brew install cmake
```

### Building the program (in MateROV folder)

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Running the program (in build folder)

```bash
./MateROV
```
