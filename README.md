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

Currently, this dynamic version of SDL is obtained through the first of these steps that is successful:
- If a shared object called `libSDL2.so` is available to
  `cosmo_dlopen`, it is loaded, and the API shims hooked into it.
- The same is attempted with the following fallback filenames, in order: `libSDL2-2.0.so`, `SDL2.dll`.
- If the environment is x86\_64 Windows, then an official build of `SDL2.dll` is
  extracted to the current directory and loaded.
- If the environment is Mac OS X, then an official build of `libSDL2.dylib` is
  extracted to the current directory and loaded.

See `sdl2/SDL_dynapi_cosmo.c` for specifics. Note that the previous process runs
the first time any SDL procedure is executed in any thread, and that failure is not
recoverable.

## How do you build this?
You will need the [cosmocc](https://github.com/jart/cosmopolitan/blob/master/tool/cosmocc/README.md)
toolchain in your path. At least version 3.2.2 is needed for proper dlopen support.
Running a compatible version of make will yield following the examples in the `o/` subdirectory, as
portable executables:
- `imgui_example.com` contains the demo of the [Dear ImGui](https://github.com/ocornut/imgui)
  immediate-mode user interface toolchain.
- `oggplay.com` is a minimal player for OGG audio, built on top of [stb\_vorbis](https://github.com/nothings/stb).

Please note that this process will download several files, including prebuilt binaries for some of the
runtime targets. The hashes of these executable files are validated with known values to ensure the
resulting artifact is reproducible.

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
- Some way for errors in the dynamic api loading to be recoverable.
