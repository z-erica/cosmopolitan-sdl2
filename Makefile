CC = cosmocc
CXX = cosmoc++
AR = cosmoar
ZIPOBJ = zipobj

CFLAGS = -Isdl2 -g -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -Io/gl3w
CXXFLAGS = $(CFLAGS)

SDL2_LIB = o/libSDL2.a
SDL2_LIB_OBJS = o/sdl2/SDL_dynapi_cosmo.o

SDL2_DLL_RELEASE = 2.28.5
SDL2_DLL = o/SDL2.dll
SDL2_DLL_ZIP = o/SDL2-$(SDL2_DLL_RELEASE)-win32-x64.zip
SDL2_DLL_ZIP_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL2_DLL_RELEASE)/SDL2-$(SDL2_DLL_RELEASE)-win32-x64.zip
# downloading opaque binaries off the internet is spooky!
# so make sure its exactly a known version
SDL2_DLL_HASH = de23db1694a3c7a4a735e7ecd3d214b2023cc2267922c6c35d30c7fc7370d677

IMGUI_EXAMPLE = o/imgui_example.com
IMGUI_EXAMPLE_OBJS = o/imgui/imgui.o \
		     o/imgui/imgui_demo.o \
		     o/imgui/imgui_draw.o \
		     o/imgui/imgui_tables.o \
		     o/imgui/imgui_widgets.o \
		     o/imgui/imgui_impl_sdl2.o \
		     o/imgui/imgui_example.o \
		     o/gl3w/gl3w.o \
		     o/SDL2.dll.zip.o

OGGPLAY_EXAMPLE = o/oggplay_example.com
OGGPLAY_EXAMPLE_OBJS = o/oggplay/main.o \
		       o/SDL2.dll.zip.o

default: $(IMGUI_EXAMPLE) $(OGGPLAY_EXAMPLE)

$(SDL2_LIB): $(SDL2_LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(IMGUI_EXAMPLE): $(IMGUI_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

$(OGGPLAY_EXAMPLE): $(OGGPLAY_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

o/gl3w/GL/gl3w.h: gl3w/gl3w_gen.py
	@mkdir -p o/gl3w
	(cd o/gl3w; python ../../gl3w/gl3w_gen.py)
o/gl3w/gl3w.c: o/gl3w/GL/gl3w.h
o/gl3w/GL/glcorearb.h: o/gl3w/GL/gl3w.h
o/gl3w/KHR/khrplatform.h: o/gl3w/GL/gl3w.h
o/imgui/imgui_example.o: o/gl3w/GL/gl3w.h

$(SDL2_DLL_ZIP):
	@mkdir -p o
	curl -L -o $@ $(SDL2_DLL_ZIP_URL)
$(SDL2_DLL): $(SDL2_DLL_ZIP)
	unzip -DD -o $< SDL2.dll -d o

o/%.zip.o: %
	@mkdir -p $(dir $@)/.aarch64
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a x86_64 -o $@ $<
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a aarch64 -o $(dir $@)/.aarch64/$(notdir $@) $<
o/%.zip.o: o/%
	@mkdir -p $(dir $@)/.aarch64
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a x86_64 -o $@ $<
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a aarch64 -o $(dir $@)/.aarch64/$(notdir $@) $<
o/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
o/%.o: o/%.c
	$(CC) -c $(CFLAGS) -o $@ $<
o/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -rf o
	rm -rf */.aarch64
