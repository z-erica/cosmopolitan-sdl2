# Cosmopolitan SDL2

## What is this?
This is a compatibility layer that aims to allow access to a native version of
[SDL][https://libsdl.org/] from portable executables linked through
[http://justine.lol/cosmopolitan/][Cosmopolitan Libc].

## How does this work?
This replaces the entire implementation of SDL with a shim which leverages its
existing [dynamic API facility][https://wiki.libsdl.org/SDL2/README/dynapi] to
hook into a version of SDL that is specific to the current runtime platform.

Currently, this dynamic version of SDL can be provided in the following ways:
- If a shared object called `libSDL2-2.0.so` is available to
  `cosmo_dlopen`, it is loaded, and the API shims hooked into it.

See `sdl2/SDL_dynapi_cosmo.c` for specifics.

## How do you build this?
You will need the [cosmocc][https://github.com/jart/cosmopolitan/blob/master/tool/cosmocc/README.md]
toolchain in your path. Running a compatible version of make will primarily
result in a portable executable named `imgui_example.com`, which contains the demo
of the [Dear ImGui][https://github.com/ocornut/imgui] immediate-mode user interface toolchain.
