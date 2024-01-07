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
#include "libc/dce.h"

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
        if (IsWindows()) jump_table.SDL_LogMessageV.ms_abi(category, SDL_LOG_PRIORITY_##prio,fmt, ap);       \
        else jump_table.SDL_LogMessageV.sysv_abi(category, SDL_LOG_PRIORITY_##prio, fmt, ap);                \
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
        if (IsWindows()) result = jump_table.SDL_vsnprintf.ms_abi(buf, sizeof(buf), fmt, ap);                                             \
        else result = jump_table.SDL_vsnprintf.sysv_abi(buf, sizeof(buf), fmt, ap);                                                       \
        va_end(ap);                                                                                                                       \
        if (result >= 0 && (size_t)result >= sizeof(buf)) {                                                                               \
            size_t len = (size_t)result + 1;                                                                                              \
            if (IsWindows()) str = (char *)jump_table.SDL_malloc.ms_abi(len);                                                             \
            else str = (char *)jump_table.SDL_malloc.sysv_abi(len);                                                                       \
            if (str) {                                                                                                                    \
                va_start(ap, fmt);                                                                                                        \
                if (IsWindows()) result = jump_table.SDL_vsnprintf.ms_abi(str, len, fmt, ap);                                             \
	        else result = jump_table.SDL_vsnprintf.sysv_abi(str, len, fmt, ap);                                                       \
                va_end(ap);                                                                                                               \
            }                                                                                                                             \
        }                                                                                                                                 \
        if (result >= 0) {                                                                                                                \
            if (IsWindows()) result = jump_table.SDL_SetError.ms_abi("%s", str);                                                          \
            else result = jump_table.SDL_SetError.sysv_abi("%s", str);                                                                    \
        }                                                                                                                                 \
        if (str != buf) {                                                                                                                 \
            if (IsWindows()) jump_table.SDL_free.ms_abi(str);                                                                             \
            else jump_table.SDL_free.sysv_abi(str);                                                                                       \
        }                                                                                                                                 \
        return result;                                                                                                                    \
    }                                                                                                                                     \
    _static int SDLCALL SDL_sscanf##name(const char *buf, SDL_SCANF_FORMAT_STRING const char *fmt, ...)                                   \
    {                                                                                                                                     \
        int retval;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        if (IsWindows()) retval = jump_table.SDL_vsscanf.ms_abi(buf, fmt, ap);                                                            \
        else retval = jump_table.SDL_vsscanf.sysv_abi(buf, fmt, ap);                                                                      \
        va_end(ap);                                                                                                                       \
        return retval;                                                                                                                    \
    }                                                                                                                                     \
    _static int SDLCALL SDL_snprintf##name(SDL_OUT_Z_CAP(maxlen) char *buf, size_t maxlen, SDL_PRINTF_FORMAT_STRING const char *fmt, ...) \
    {                                                                                                                                     \
        int retval;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        if (IsWindows()) retval = jump_table.SDL_vsnprintf.ms_abi(buf, maxlen, fmt, ap);                                                  \
        else retval = jump_table.SDL_vsnprintf.sysv_abi(buf, maxlen, fmt, ap);                                                            \
        va_end(ap);                                                                                                                       \
        return retval;                                                                                                                    \
    }                                                                                                                                     \
    _static int SDLCALL SDL_asprintf##name(char **strp, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)                                    \
    {                                                                                                                                     \
        int retval;                                                                                                                       \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        if (IsWindows()) retval = jump_table.SDL_vasprintf.ms_abi(strp, fmt, ap);                                                         \
        else retval = jump_table.SDL_vasprintf.sysv_abi(strp, fmt, ap);                                                                   \
        va_end(ap);                                                                                                                       \
        return retval;                                                                                                                    \
    }                                                                                                                                     \
    _static void SDLCALL SDL_Log##name(SDL_PRINTF_FORMAT_STRING const char *fmt, ...)                                                     \
    {                                                                                                                                     \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        if (IsWindows()) jump_table.SDL_LogMessageV.ms_abi(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);                 \
        else jump_table.SDL_LogMessageV.sysv_abi(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);                           \
        va_end(ap);                                                                                                                       \
    }                                                                                                                                     \
    _static void SDLCALL SDL_LogMessage##name(int category, SDL_LogPriority priority, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)      \
    {                                                                                                                                     \
        va_list ap;                                                                                                                       \
        initcall;                                                                                                                         \
        va_start(ap, fmt);                                                                                                                \
        if (IsWindows()) jump_table.SDL_LogMessageV.ms_abi(category, priority, fmt, ap);                                                  \
        else jump_table.SDL_LogMessageV.sysv_abi(category, priority, fmt, ap);                                                            \
        va_end(ap);                                                                                                                       \
    }                                                                                                                                     \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Verbose, VERBOSE)                                                                   \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Debug, DEBUG)                                                                       \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Info, INFO)                                                                         \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Warn, WARN)                                                                         \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Error, ERROR)                                                                       \
    SDL_DYNAPI_VARARGS_LOGFN(_static, name, initcall, Critical, CRITICAL)

/* Typedefs for function pointers for jump table */
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret)               \
    typedef union {                                              \
	void *ptr;                                               \
        rc (*sysv_abi) params;                           \
	rc (*__attribute__((__ms_abi__)) ms_abi) params; \
    } SDL_DYNAPIFN_##fn;
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
static SDL_DYNAPI_jump_table jump_table = { 0 };

static void InitCosmoDynamicAPI(void);

/* Public API functions to jump into the jump table. */
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret)      \
    rc SDLCALL fn params                                \
    {                                                   \
        InitCosmoDynamicAPI();                          \
	if (IsWindows()) ret jump_table.fn.ms_abi args; \
	else ret jump_table.fn.sysv_abi args;           \
    }
#define SDL_DYNAPI_PROC_NO_VARARGS 1
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
#undef SDL_DYNAPI_PROC_NO_VARARGS
SDL_DYNAPI_VARARGS(, , InitCosmoDynamicAPI())

static void InitCosmoDynamicAPIOnce(void)
{
    char *libname = "libSDL2-2.0.so";
    void *lib = cosmo_dlopen(libname, RTLD_LAZY | RTLD_LOCAL);

    if (lib == NULL) {
        kprintf("error: failed to load native SDL: %s\n", cosmo_dlerror());
	abort();
    }

#define SDL_DYNAPI_PROC(rc, fn, params, args, ret)                         \
    jump_table.fn.ptr = cosmo_dlsym(lib, #fn);                             \
    if (jump_table.fn.ptr == NULL) {                                       \
        kprintf("warning: failed to load symbol from native SDL: %s\n",    \
	    #fn, cosmo_dlerror());                                         \
	abort();                                                           \
    }                                                                      \
    if (!IsWindows()) jump_table.fn.ptr = cosmo_dltramp(jump_table.fn.ptr);
#include "SDL_dynapi_procs.inc"
#undef SDL_DYNAPI_PROC
}

static void InitCosmoDynamicAPI(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, InitCosmoDynamicAPIOnce);
}
