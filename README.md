# Cosmopolitan SDL2

## What is this?
This is a compatibility layer that aims to allow access to a native version of
[SDL](https://libsdl.org/) from portable executables linked through
[Cosmopolitan Libc](http://justine.lol/cosmopolitan/). The provided API is pinned
to SDL 2.0.9.

## How does this work?
This replaces the entire implementation of SDL with a shim which leverages its
existing [dynamic API facility](https://wiki.libsdl.org/SDL2/README/dynapi) to
hook into a version of SDL that is specific to the current runtime platform.

Currently, this dynamic version of SDL can be provided in the following ways:
- If a shared object called `libSDL2-2.0.so` is available to
  `cosmo_dlopen`, it is loaded, and the API shims hooked into it.

See `sdl2/SDL_dynapi_cosmo.c` for specifics.

## How do you build this?
You will need the [cosmocc](https://github.com/jart/cosmopolitan/blob/master/tool/cosmocc/README.md)
toolchain in your path. At least version 3.2.2 is needed for proper dlopen support.
Running a compatible version of make will yield following the examples in the `o/` subdirectory, as
portable executables:
- `imgui_example.com` contains the demo of the [Dear ImGui](https://github.com/ocornut/imgui)
  immediate-mode user interface toolchain.
- `oggplay.com` is a minimal player for OGG audio, built on top of [stb\_vorbis](https://github.com/nothings/stb).

## The OpenGL loader
To minimize the requirements on the native SDL implementation, the Dear ImGui demo is not bundled with its
SDL\_Renderer backend. Instead, it includes the OpenGL 3.0 rendering backend, which means that as part of its
setup, it is necessary to load the necessary function pointers. SDL provides this functionality through
`SDL_GL_GetProcAddress`. Naturally, the returned pointers require the same treatment as the hooks in
`sdl2/SDL_dynapi_cosmo.c`. A modified version of [gl3w](https://github.com/skaslev/gl3w) generates code that
transparently dispatches between calling conventions.

## Ideas for improvement
- Implementing a proper chain of fallbacks for the shared object filename. A `SDL_COSMO_API`
  override environment variable, separate from `SDL_DYNAMIC_API` (which, if set,
  would apply to the library loaded by cosmo and not to this wrapper).
- Bundling a prebuilt native library for systems where it makes more sense (Windows).
  This could potentially make fully self contained source builds more awkward, however.
- Testing real code with callbacks from SDL.
