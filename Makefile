CC = cosmocc
CXX = cosmoc++
AR = cosmoar
CXXFLAGS = -Isdl2

SDL2_LIB = libSDL2.a
SDL2_LIB_OBJS = sdl2/SDL_dynapi_cosmo.o
IMGUI_EXAMPLE = imgui_example.com
IMGUI_EXAMPLE_OBJS = imgui/imgui.o \
		     imgui/imgui_demo.o \
		     imgui/imgui_draw.o \
		     imgui/imgui_example.o \
		     imgui/imgui_impl_sdl2.o \
		     imgui/imgui_impl_sdlrenderer2.o \
		     imgui/imgui_tables.o \
		     imgui/imgui_widgets.o

$(IMGUI_EXAMPLE): $(IMGUI_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

$(SDL2_LIB): $(SDL2_LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -f $(SDL2_LIB) $(SDL2_LIB_OBJS) $(IMGUI_EXAMPLE) $(IMGUI_EXAMPLE_OBJS)
	rm -f *.dbg *.aarch64.elf
