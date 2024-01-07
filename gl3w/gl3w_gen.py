#!/usr/bin/env python

#   This file is part of gl3w, hosted at https://github.com/skaslev/gl3w
#
#   This is free and unencumbered software released into the public domain.
#
#   Anyone is free to copy, modify, publish, use, compile, sell, or
#   distribute this software, either in source code form or as a compiled
#   binary, for any purpose, commercial or non-commercial, and by any
#   means.
#
#   In jurisdictions that recognize copyright laws, the author or authors
#   of this software dedicate any and all copyright interest in the
#   software to the public domain. We make this dedication for the benefit
#   of the public at large and to the detriment of our heirs and
#   successors. We intend this dedication to be an overt act of
#   relinquishment in perpetuity of all present and future rights to this
#   software under copyright law.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#   OTHER DEALINGS IN THE SOFTWARE.

# modified for cosmopolitan-sdl2
# January 2024, Erica Z <zerica@callcc.eu>

# Allow Python 2.6+ to use the print() function
from __future__ import print_function

import argparse
import os
import re

# Try to import Python 3 library urllib.request
# and if it fails, fall back to Python 2 urllib2
try:
    import urllib.request as urllib2
except ImportError:
    import urllib2

# UNLICENSE copyright header
UNLICENSE = r'''/*
 * This file was generated with gl3w_gen.py, part of gl3w
 * (hosted at https://github.com/skaslev/gl3w)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

'''

EXT_SUFFIX = ['ARB', 'EXT', 'KHR', 'OVR', 'NV', 'AMD', 'INTEL']

def is_ext(proc):
    return any(proc.endswith(suffix) for suffix in EXT_SUFFIX)

def write(f, s):
    f.write(s.encode('utf-8'))

def touch_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def download(url, dst):
    if os.path.exists(dst):
        print('Reusing {0}...'.format(dst))
        return

    print('Downloading {0}...'.format(dst))
    web = urllib2.urlopen(urllib2.Request(url, headers={'User-Agent': 'Mozilla/5.0'}))
    with open(dst, 'wb') as f:
        f.writelines(web.readlines())

parser = argparse.ArgumentParser(description='gl3w generator script')
parser.add_argument('--ext', action='store_true', help='Load extensions')
parser.add_argument('--root', type=str, default='', help='Root directory')
args = parser.parse_args()

# Create directories
touch_dir(os.path.join(args.root, 'GL'))
touch_dir(os.path.join(args.root, 'KHR'))

# Download glcorearb.h and khrplatform.h
download('https://registry.khronos.org/OpenGL/api/GL/glcorearb.h',
         os.path.join(args.root, 'GL/glcorearb.h'))
download('https://registry.khronos.org/EGL/api/KHR/khrplatform.h',
         os.path.join(args.root, 'KHR/khrplatform.h'))

# Parse function names from glcorearb.h
print('Parsing glcorearb.h header...')
procs = []
# This regular expression extracts the following information
# from each GL function declaration as a tuple:
#   (return_type, name, arglist)
p = re.compile(r'GLAPI\s+(.+?)APIENTRY\s+(\w+)[^(]*([^;]+);')
# This regular expression matches the parameter names in the
# arglist. When applied trough re.sub, it performs the following
# substitution:
#   '(int one, void *two)' -> 'one, two)'
# Then, 
q = re.compile(r'[^,)]*?(\w+[,)])')
with open(os.path.join(args.root, 'GL/glcorearb.h'), 'r') as f:
    for line in f:
        m = p.findall(line)
        if len(m) == 0:
            continue
        proc = m[0]
        # proc is a tuple as seen above above; (return_type, name, arglist)
        # the following code inserts two extra elements:
        # - a list of parameter names. that is, arglist with the types stripped
        # - the string 'return' if the function returns a value, and '' otherwise
        # these will fill out the shim that dispatches between calling conventions
        proc = proc + ('()' if proc[2] == '(void)' else '('+q.sub(r'\1', proc[2]),
                       '' if proc[0].strip() == 'void' else 'return')
        if args.ext or not is_ext(proc[1]):
            procs.append(proc)
procs.sort()

# Generate gl3w.h
print('Generating {0}...'.format(os.path.join(args.root, 'GL/gl3w.h')))
with open(os.path.join(args.root, 'GL/gl3w.h'), 'wb') as f:
    write(f, UNLICENSE)
    write(f, r'''#ifndef __gl3w_h_
#define __gl3w_h_

#include <GL/glcorearb.h>

#define _COSMO_SOURCE
#include "libc/dce.h"

#ifndef GL3W_API
#define GL3W_API
#endif

#ifndef __gl_h_
#define __gl_h_
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GL3W_OK 0
#define GL3W_ERROR_INIT -1
#define GL3W_ERROR_LIBRARY_OPEN -2
#define GL3W_ERROR_OPENGL_VERSION -3

/* gl3w api */
GL3W_API void gl3wInit(void);

/* gl3w internal state */
''')
    write(f, 'union GL3WProcs {\n')
    write(f, '\tvoid *ptr[{0}];\n'.format(len(procs)))
    write(f, '\tstruct {\n')
    for proc in procs:
        write(f, '\t\t{0}(* {1}) {2};\n'.format(*proc))
    write(f, '	} sysv;\n')
    write(f, '\tstruct {\n')
    for proc in procs:
        write(f, '\t\t{0}(* __attribute__((__ms_abi__)) {1}) {2};\n'.format(*proc))
    write(f, r'''	} ms;
};

GL3W_API extern union GL3WProcs gl3wProcs;

/* OpenGL functions */
''')
    for proc in procs:
        write(f, 'static {0}{1}{2}\n'.format(*proc))
        write(f, '{{ if (IsWindows()) {4}   gl3wProcs.ms.{1}{3};\n'.format(*proc))
        write(f, '  else             {4} gl3wProcs.sysv.{1}{3}; }}\n\n'.format(*proc))
    write(f, r'''
#ifdef __cplusplus
}
#endif

#endif
''')

# Generate gl3w.c
print('Generating {0}...'.format(os.path.join(args.root, 'gl3w.c')))
with open(os.path.join(args.root, 'gl3w.c'), 'wb') as f:
    write(f, UNLICENSE)
    write(f, r'''#include <GL/gl3w.h>
#include "SDL.h"
#include "libc/dlopen/dlfcn.h"

static const char *proc_names[] = {
''')
    for proc in procs:
        write(f, '\t"{0}",\n'.format(proc[1]))
    write(f, r'''};

GL3W_API union GL3WProcs gl3wProcs;

void gl3wInit(void)
{
	size_t i;

	for (i = 0; i < sizeof(proc_names)/sizeof(proc_names[0]); i++) {
		gl3wProcs.ptr[i] = SDL_GL_GetProcAddress(proc_names[i]);
        if (!IsWindows()) gl3wProcs.ptr[i] = cosmo_dltramp(gl3wProcs.ptr[i]);
    }
}
''')
