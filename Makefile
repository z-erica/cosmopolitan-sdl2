CC = cosmocc
CXX = cosmoc++
AR = cosmoar
ZIPOBJ = zipobj

CFLAGS = -Isdl2 -g -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -Io/gl3w
CXXFLAGS = $(CFLAGS)

SDL2_LIB = o/libSDL2.a
SDL2_LIB_OBJS = o/sdl2/SDL_dynapi_cosmo.o

SDL2_BUNDLED_RELEASE = 2.28.5

SDL2_DLL = o/SDL2.dll
SDL2_DLL_ZIP = o/SDL2-$(SDL2_BUNDLED_RELEASE)-win32-x64.zip
SDL2_DLL_ZIP_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL2_BUNDLED_RELEASE)/SDL2-$(SDL2_BUNDLED_RELEASE)-win32-x64.zip
# downloading opaque binaries off the internet is spooky!
# so make sure its exactly a known version
SDL2_DLL_HASH = de23db1694a3c7a4a735e7ecd3d214b2023cc2267922c6c35d30c7fc7370d677

SDL2_DYLIB = o/libSDL2.dylib
SDL2_DYLIB_DMG = o/SDL2-$(SDL2_BUNDLED_RELEASE).dmg
SDL2_DYLIB_DMG_URL = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL2_BUNDLED_RELEASE)/SDL2-$(SDL2_BUNDLED_RELEASE).dmg
SDL2_DYLIB_HASH = cea415afd6d89a926478aad6be400760f20a28506e872641a17856582a84edfe

SDL2_BUNDLED_OBJS = o/SDL2.dll.zip.o o/libSDL2.dylib.zip.o

IMGUI_EXAMPLE = o/imgui_example.com
IMGUI_EXAMPLE_OBJS = o/imgui/imgui.o \
		     o/imgui/imgui_demo.o \
		     o/imgui/imgui_draw.o \
		     o/imgui/imgui_tables.o \
		     o/imgui/imgui_widgets.o \
		     o/imgui/imgui_impl_sdl2.o \
		     o/imgui/imgui_example.o \
		     o/gl3w/gl3w.o \
		     $(SDL2_BUNDLED_OBJS)

OGGPLAY_EXAMPLE = o/oggplay_example.com
OGGPLAY_EXAMPLE_OBJS = o/oggplay/main.o \
		       $(SDL2_BUNDLED_OBJS)

UXNEMU = o/uxnemu.com
UXNEMU_OBJS = o/uxn/devices/datetime.o \
							o/uxn/devices/system.o \
							o/uxn/devices/console.o \
							o/uxn/devices/file.o \
							o/uxn/devices/audio.o \
							o/uxn/devices/controller.o \
							o/uxn/devices/mouse.o \
							o/uxn/devices/screen.o \
							o/uxn/uxn.o \
							o/uxn/uxnemu.o \
							$(SDL2_BUNDLED_OBJS)

default: $(IMGUI_EXAMPLE) $(OGGPLAY_EXAMPLE) $(UXNEMU)

$(SDL2_LIB): $(SDL2_LIB_OBJS)
	$(AR) r $@ $^

$(IMGUI_EXAMPLE): $(IMGUI_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

$(OGGPLAY_EXAMPLE): $(OGGPLAY_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

$(UXNEMU): $(UXNEMU_OBJS) $(SDL2_LIB)
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
	curl -sL -o $@ $(SDL2_DLL_ZIP_URL)
$(SDL2_DLL): $(SDL2_DLL_ZIP)
	unzip -q -DD -o $< SDL2.dll -d o

$(SDL2_DYLIB_DMG):
	@mkdir -p o
	curl -sL -o $@ $(SDL2_DYLIB_DMG_URL)
$(SDL2_DYLIB): $(SDL2_DYLIB_DMG)
	7z e $< SDL2/SDL2.framework/Versions/A/SDL2 -oo
	mv o/SDL2 o/libSDL2.dylib
	touch o/libSDL2.dylib

o/SDL2.dll.zip.o: o/SDL2.dll
	@mkdir -p $(dir $@)/.aarch64
	@echo '$(SDL2_DLL_HASH)  o/SDL2.dll' | sha256sum --check --quiet -
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a x86_64 -o $@ -C 1 $<
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a aarch64 -o $(dir $@)/.aarch64/$(notdir $@) -C 1 $<
o/libSDL2.dylib.zip.o: o/libSDL2.dylib
	@mkdir -p $(dir $@)/.aarch64
	@echo '$(SDL2_DYLIB_HASH)  o/libSDL2.dylib' | sha256sum --check --quiet -
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a x86_64 -o $@ -C 1 $<
	$(ZIPOBJ) $(ZIPOBJ_FLAGS) -a aarch64 -o $(dir $@)/.aarch64/$(notdir $@) -C 1 $<

o/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
o/%.o: o/%.c
	$(CC) -c $(CFLAGS) -o $@ $<
o/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -rf o
