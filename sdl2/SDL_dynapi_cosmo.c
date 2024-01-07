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
#include "libc/runtime/runtime.h"
#include "libc/thread/thread.h"
#include "libc/dlopen/dlfcn.h"
#include "libc/dce.h"
#include "libc/assert.h"
#include "libc/isystem/fcntl.h"
#include "libc/str/str.h"
#include "libc/temp.h"
#include "libc/errno.h"
#include "libc/log/log.h"

#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_vulkan.h"

/* all the procedures listed here necessarily rely on callbacks. mask them while we dont have a
 * prettier way to deal with them */
#define SDL_SetAssertionHandler SDL_SetAssertionHandler_REAL
#define SDL_SetEventFilter SDL_SetEventFilter_REAL
#define SDL_AddEventWatch SDL_AddEventWatch_REAL
#define SDL_FilterEvents SDL_FilterEvents_REAL
#define SDL_AddHintCallback SDL_AddHintCallback_REAL
#define SDL_LogSetOutputFunction SDL_LogSetOutputFunction_REAL
#define SDL_SetMemoryFunctions SDL_SetMemoryFunctions_REAL
#define SDL_CreateThread SDL_CreateThread_REAL
#define SDL_CreateThreadWithStackSize SDL_CreateThreadWithStackSize_REAL
#define SDL_AddTimer SDL_AddTimer_REAL
#define SDL_SetWindowHitTest SDL_SetWindowHitTest_REAL

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

/* NOTE: i do not know enough to make this obviously efficient */
/* its perfectly possible there are also correctness issues */
static void ExtractFromZip(int from_fd, int to_fd) {
    long offset;
    long bytes_read, bytes_written;
    char buffer[1024];

    offset = 0;
    while (1) {
        bytes_read = pread(from_fd, buffer, sizeof buffer, offset);
	if (bytes_read == 0) break; /* eof */
	assert(bytes_read > 0);

	bytes_written = pwrite(to_fd, buffer, bytes_read, offset);
	/* FIXME: if something breaks, it may be because of this assumption */
	assert(bytes_written == bytes_read);

	offset += bytes_written;
    }
}

static int TryDLLExtractPath(int from_fd, const char *to) {
    int to_fd;

    to_fd = open(to, O_WRONLY|O_CREAT, S_IRWXU);
    if (to_fd < 0) {
        FWARNF(stderr, "could not extract SDL2.dll to %s: %s", to, strerror(errno));
	return 0;
    }

    ExtractFromZip(from_fd, to_fd);

    assert(close(to_fd) >= 0);
    return 1;
}

static void *ExtractDLL_x86_64(void)
{
    int from_fd = open("/zip/SDL2.dll", O_RDONLY);
    if (from_fd < 0) {
        FFATALF(stderr, "could not open bundled SDL2.dll: %s", strerror(errno));
    }

    /* for now, just try the current directory */
    if (TryDLLExtractPath(from_fd, "SDL2.dll")) {
        close(from_fd);
    	return cosmo_dlopen("./SDL2.dll", RTLD_LAZY | RTLD_LOCAL);
    } else {
        FFATALF(stderr, "ran out of paths to extract SDL2.dll to");
    }
}

static void InitCosmoDynamicAPIOnce(void)
{
    const char *libname_cascade[] = {
	    "libSDL2-2.0.so",
	    "SDL2.dll",
	    NULL
    };
    const char **libname;
    void *lib = NULL;

    for (libname = libname_cascade; *libname; libname+=1) {
        if ((lib = cosmo_dlopen(*libname, RTLD_LAZY | RTLD_LOCAL))) {
	    FLOGF(stderr, "found native SDL with filename %s", *libname);
            break;
	} else {
	    FLOGF(stderr, "could not load native SDL with filename %s: %s",
			    *libname, cosmo_dlerror());
	}
    }

    if (lib == NULL && IsWindows()) {
#ifdef __x86_64__
        lib = ExtractDLL_x86_64();
#else
	FFATALF(stderr, "bundled SDL2.dll for aarch64 not yet supported");
#endif
	if (lib == NULL) {
	    FFATALF(stderr, "could not load bundled SDL2.dll after extracting: %s",
			    cosmo_dlerror());
	}
    }

    if (lib == NULL) {
        FFATALF(stderr, "failed to load native SDL");
    }

#define SDL_DYNAPI_PROC(rc, fn, params, args, ret)                         \
    jump_table.fn.ptr = cosmo_dlsym(lib, #fn);                             \
    if (jump_table.fn.ptr == NULL) {                                       \
        FWARNF(stderr, "failed to load symbol %s from native SDL: %s",     \
	    #fn, cosmo_dlerror());                                         \
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


