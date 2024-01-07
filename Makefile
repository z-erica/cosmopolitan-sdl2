CC = cosmocc
CXX = cosmoc++
AR = cosmoar
CFLAGS = -Isdl2 -g -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -Igl3w
CXXFLAGS = $(CFLAGS)

SDL2_LIB = o/libSDL2.a
SDL2_LIB_OBJS = sdl2/SDL_dynapi_cosmo.o

SDL2_DLL_RELEASE = 2.28.5
SDL2_DLL = o/SDL2.dll
SDL2_DLL_ZIP = o/SDL2-$(SDL2_DLL_RELEASE)-win32-x64.zip
SDL2_DLL_ZIP_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL2_DLL_RELEASE)/SDL2-$(SDL2_DLL_RELEASE)-win32-x64.zip
# downloading opaque binaries off the internet is spooky!
# so make sure its exactly a known version
SDL2_DLL_HASH = de23db1694a3c7a4a735e7ecd3d214b2023cc2267922c6c35d30c7fc7370d677

IMGUI_EXAMPLE = o/imgui_example.com
IMGUI_EXAMPLE_OBJS = imgui/imgui.o \
		     imgui/imgui_demo.o \
		     imgui/imgui_draw.o \
		     imgui/imgui_tables.o \
		     imgui/imgui_widgets.o \
		     imgui/imgui_impl_sdl2.o \
		     imgui/imgui_example.o \
		     gl3w/gl3w.o \
		     o/SDL2.dll.zip.o

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

$(SDL2_DLL_ZIP):
	mkdir -p o
	curl -L -o $@ $(SDL2_DLL_ZIP_URL)
$(SDL2_DLL): $(SDL2_DLL_ZIP)
	unzip -DD -o $< SDL2.dll -d o
o/SDL2.dll.zip.o: $(SDL2_DLL)
	mkdir -p o/.aarch64
	echo "$(SDL2_DLL_HASH) $(SDL2_DLL)" | sha256sum --check --quiet
	zipobj -a x86_64 -o $@ -C 1 $<
	zipobj -a aarch64 -o o/.aarch64/SDL2.dll.zip.o -C 1 $<

clean:
	rm -f $(SDL2_LIB_OBJS) $(OGGPLAY_EXAMPLE_OBJS)
	rm -f $(IMGUI_EXAMPLE_OBJS) $(GL3W_GEN_ARTIFACTS)
	rm -rf o
	rm -rf */.aarch64
