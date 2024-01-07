CC = cosmocc
CXX = cosmoc++
AR = cosmoar
CFLAGS = -Isdl2 -g -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -Igl3w
CXXFLAGS = $(CFLAGS)

SDL2_LIB = o/libSDL2.a
SDL2_LIB_OBJS = sdl2/SDL_dynapi_cosmo.o

IMGUI_EXAMPLE = o/imgui_example.com
IMGUI_EXAMPLE_OBJS = imgui/imgui.o \
		     imgui/imgui_demo.o \
		     imgui/imgui_draw.o \
		     imgui/imgui_tables.o \
		     imgui/imgui_widgets.o \
		     imgui/imgui_impl_sdl2.o \
		     imgui/imgui_example.o \
		     gl3w/gl3w.o

GL3W_GEN_ARTIFACTS = gl3w/GL/gl3w.h \
		     gl3w/gl3w.c \
		     gl3w/GL/glcorearb.h \
		     gl3w/KHR/khrplatform.h

OGGPLAY_EXAMPLE = o/oggplay_example.com
OGGPLAY_EXAMPLE_OBJS = oggplay/main.o

default: $(IMGUI_EXAMPLE) $(OGGPLAY_EXAMPLE)

$(SDL2_LIB): $(SDL2_LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(IMGUI_EXAMPLE): $(IMGUI_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

$(OGGPLAY_EXAMPLE): $(OGGPLAY_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

gl3w/GL/gl3w.h: gl3w/gl3w_gen.py
	(cd gl3w; python gl3w_gen.py)
gl3w/gl3w.c: gl3w/GL/gl3w.h
gl3w/GL/glcorearb.h: gl3w/GL/gl3w.h
gl3w/KHR/khrplatform.h: gl3w/GL/gl3w.h
imgui/imgui_example.cpp: gl3w/GL/gl3w.h

clean:
	rm -f $(SDL2_LIB_OBJS) $(OGGPLAY_EXAMPLE_OBJS)
	rm -f $(IMGUI_EXAMPLE_OBJS) $(GL3W_GEN_ARTIFACTS)
	rm -rf o
	rm -rf */.aarch64
