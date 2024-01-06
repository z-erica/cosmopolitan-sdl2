/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*
  This is a modified version of SDL_dynapi.c that replaces the entire
  implementation of the SDL2 library with a custom loader specific to
  Justine Tunney's Cosmopolitan Libc.

  January 2024, Erica Z <zerica@callcc.eu>
*/

#define _COSMO_SOURCE
#include "libc/intrin/kprintf.h"
#include "libc/runtime/runtime.h"
#include "libc/thread/thread.h"
#include "libc/dlopen/dlfcn.h"

#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_vulkan.h"

/* Can't use the macro for varargs nonsense. This is atrocious. */
#define SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, logname, prio)                                     \
    _static void SDLCALL SDL_Log##logname##name(int category, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) \
    {                                                                                                        \
        va_list ap;                                                                                          \
        initcall;                                                                                            \
        va_start(ap, fmt);                                                                                   \
        jump_table.SDL_LogMessageV(category, SDL_LOG_PRIORITY_##prio, fmt, ap);                              \
        va_end(ap);                                                                                          \
    }

#define SDL_DYNAPI_VARARGS(_static, name, initcall)                                                                                       \
    _static int SDLCALL SDL_SetError##name(SDL_PRINTF_FORMAT_STRING const char *fmt, ...)                                                 \
    {                                                                                                                                     \
        char buf[128], *str = buf;                                                                                                        \
        int result;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        result = jump_table.SDL_vsnprintf(buf, sizeof(buf), fmt, ap);                                                                     \
        va_end(ap);                                                                                                                       \
        if (result >= 0 && (size_t)result >= sizeof(buf)) {                                                                               \
            size_t len = (size_t)result + 1;                                                                                              \
            str = (char *)jump_table.SDL_malloc(len);                                                                                     \
            if (str) {                                                                                                                    \
                va_start(ap, fmt);                                                                                                        \
                result = jump_table.SDL_vsnprintf(str, len, fmt, ap);                                                                     \
                va_end(ap);                                                                                                               \
            }                                                                                                                             \
        }                                                                                                                                 \
        if (result >= 0) {                                                                                                                \
            result = jump_table.SDL_SetError("%s", str);                                                                                  \
        }                                                                                                                                 \
        if (str != buf) {                                                                                                                 \
            jump_table.SDL_free(str);                                                                                                     \
        }                                                                                                                                 \
        return result;                                                                                                                    \
    }                                                                                                                                     \
    _static int SDLCALL SDL_sscanf##name(const char *buf, SDL_SCANF_FORMAT_STRING const char *fmt, ...)                                   \
    {                                                                                                                                     \
        int retval;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        retval = jump_table.SDL_vsscanf(buf, fmt, ap);                                                                                    \
        va_end(ap);                                                                                                                       \
        return retval;                                                                                                                    \
    }                                                                                                                                     \
    _static int SDLCALL SDL_snprintf##name(SDL_OUT_Z_CAP(maxlen) char *buf, size_t maxlen, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) \
    {                                                                                                                                     \
        int retval;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        retval = jump_table.SDL_vsnprintf(buf, maxlen, fmt, ap);                                                                          \
        va_end(ap);                                                                                                                       \
        return retval;                                                                                                                    \
    }                                                                                                                                     \
    _static int SDLCALL SDL_asprintf##name(char **strp, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)                                    \
    {                                                                                                                                     \
        int retval;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        retval = jump_table.SDL_vasprintf(strp, fmt, ap);                                                                                 \
        va_end(ap);                                                                                                                       \
        return retval;                                                                                                                    \
    }                                                                                                                                     \
    _static void SDLCALL SDL_Log##name(SDL_PRINTF_FORMAT_STRING const char *fmt, ...)                                                     \
    {                                                                                                                                     \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        jump_table.SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);                                         \
        va_end(ap);                                                                                                                       \
    }                                                                                                                                     \
    _static void SDLCALL SDL_LogMessage##name(int category, SDL_LogPriority priority, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)      \
    {                                                                                                                                     \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        jump_table.SDL_LogMessageV(category, priority, fmt, ap);                                                                          \
        va_end(ap);                                                                                                                       \
    }                                                                                                                                     \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Verbose, VERBOSE)                                                                   \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Debug, DEBUG)                                                                       \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Info, INFO)                                                                         \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Warn, WARN)                                                                         \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Error, ERROR)                                                                       \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Critical, CRITICAL)

/* Typedefs for function pointers for jump table, and predeclare funcs */
/* The DEFAULT funcs will init jump table and then call real function. */
/* The REAL funcs are the actual functions, name-mangled to not clash. */
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    typedef rc (SDLCALL *SDL_DYNAPIFN_##fn) params;\
    static rc SDLCALL fn##_DEFAULT params;         \
    extern rc SDLCALL fn##_REAL params;
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC

/* The jump table! */
typedef struct
{
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) SDL_DYNAPIFN_##fn fn;
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
} SDL_DYNAPI_jump_table;

/* Predeclare the default functions for initializing the jump table. */
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) static rc SDLCALL fn##_DEFAULT params;
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC

/* The actual jump table. */
static SDL_DYNAPI_jump_table jump_table = {
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) fn##_DEFAULT,
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
};

static void InitCosmoDynamicAPI(void);

/* Default functions init the function table then call right thing. */
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    static rc SDLCALL fn##_DEFAULT params          \
    {                                              \
        InitCosmoDynamicAPI();                      \
        ret jump_table.fn args;                    \
    }
#define SDL_DYNAPI_PROC_NO_VARARGS 1
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
#undef SDL_DYNAPI_PROC_NO_VARARGS
SDL_DYNAPI_VARARGS(static, _DEFAULT, InitCosmoDynamicAPI())

/* Public API functions to jump into the jump table. */
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    rc SDLCALL fn params                           \
    {                                              \
        ret jump_table.fn args;                    \
    }
#define SDL_DYNAPI_PROC_NO_VARARGS 1
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
#undef SDL_DYNAPI_PROC_NO_VARARGS
SDL_DYNAPI_VARARGS(, , )

static void InitCosmoDynamicAPIOnce(void)
{
    char *libname = "libSDL2-2.0.so";
    void *lib = cosmo_dlopen(libname, RTLD_LAZY | RTLD_LOCAL);

    if (lib == NULL) {
        kprintf("error: failed to load native SDL: %s\n", cosmo_dlerror());
	abort();
    }

#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    jump_table.fn = cosmo_dlsym(lib, #fn); \
    if (jump_table.fn == NULL) { \
        kprintf("warning: failed to load symbol from native SDL: %s\n", \
	    #fn, cosmo_dlerror()); \
	abort(); \
    }
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
}

static void InitCosmoDynamicAPI(void)
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	pthread_once(&once, InitCosmoDynamicAPIOnce);
}
