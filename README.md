# Cosmopolitan SDL2

## What is this?
This is a compatibility layer that aims to allow access to a native version of
[SDL](https://libsdl.org/) from portable executables linked through
[Cosmopolitan Libc](http://justine.lol/cosmopolitan/).

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
toolchain in your path. At least version 3.2.1 is needed for proper dlopen support.
Running a compatible version of make will yield following the examples in the `o/` subdirectory, as
portable executables:
- `imgui_example.com` contains the demo of the [Dear ImGui](https://github.com/ocornut/imgui)
  immediate-mode user interface toolchain.
- `oggplay.com` is a minimal player for OGG audio, built on top of [stb\_vorbis](https://github.com/nothings/stb).

## Ideas for improvement
- Implementing a proper chain of fallbacks for the shared object filename. A `SDL_COSMO_API`
  override environment variable, separate from `SDL_DYNAMIC_API` (which, if set,
  would apply to the library loaded by cosmo and not to this wrapper).
- Bundling a prebuilt native library for systems where it makes more sense (Windows).
  This could potentially make fully self contained source builds more awkward, however.
- The indirection is currently double; these wrappers are indirect jumps through a
  jump table, which hooks into the native library's own dynamic API wrappers. This is in
  constrast to the original SDL API, which calls into the overriding DLL to remotely rewrite
  the calling application's jump table to its own actual implementations (see `SDL_DYNAPI_entry`
  in `SDL_dynapi.c`). I'm unsure whether emulating this would be much of a benefit, considering
  there's a pretty heavy overhead to `cosmo_dlopen` already.
- A similarly spirited OpenGL shim (CosmoGLEW?).
- Testing real code with callbacks from SDL.
