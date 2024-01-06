CC = cosmocc
CXX = cosmoc++
AR = cosmoar
CFLAGS = -Isdl2 -g
CXXFLAGS = $(CFLAGS)

SDL2_LIB = o/libSDL2.a
SDL2_LIB_OBJS = sdl2/SDL_dynapi_cosmo.o

IMGUI_EXAMPLE = o/imgui_example.com
IMGUI_EXAMPLE_OBJS = imgui/imgui.o \
		     imgui/imgui_demo.o \
		     imgui/imgui_draw.o \
		     imgui/imgui_example.o \
		     imgui/imgui_impl_sdl2.o \
		     imgui/imgui_impl_sdlrenderer2.o \
		     imgui/imgui_tables.o \
		     imgui/imgui_widgets.o

OGGPLAY_EXAMPLE = o/oggplay_example.com
OGGPLAY_EXAMPLE_OBJS = oggplay/main.o

default: $(IMGUI_EXAMPLE) $(OGGPLAY_EXAMPLE)

$(SDL2_LIB): $(SDL2_LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(IMGUI_EXAMPLE): $(IMGUI_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

$(OGGPLAY_EXAMPLE): $(OGGPLAY_EXAMPLE_OBJS) $(SDL2_LIB)
	$(CC) $(LDLIBS) -o $@ $^

clean:
	rm -f $(SDL2_LIB_OBJS) $(IMGUI_EXAMPLE_OBJS) $(OGGPLAY_EXAMPLE_OBJS)
	rm -f o
