#
# Cross Platform Makefile
#

# Compiler
#CXX = g++
#CXX = clang++
RC = windres

EXE = MugenSpriteViewer.exe

# Resource
RES_DIR = res
RCFILE = $(RES_DIR)/mugen.rc
RESFILE = $(RES_DIR)/mugen_res.o
ICON = $(RES_DIR)/mugen.ico

# Source directories
SRC_DIR = src
IMGUI_DIR = $(SRC_DIR)/imgui
IMGUI_BACKENDS_DIR = $(IMGUI_DIR)/backends
GLAD_DIR = $(SRC_DIR)/glad
MUGEN_DIR = $(SRC_DIR)/mugen
LODEPNG_DIR = $(SRC_DIR)/lodepng

# Source files
SOURCES = \
	$(SRC_DIR)/main.cpp \
	$(GLAD_DIR)/glad.c \
	$(MUGEN_DIR)/mugen_sff.cpp \
	$(LODEPNG_DIR)/lodepng.cpp \
	$(IMGUI_DIR)/imgui.cpp \
	$(IMGUI_DIR)/imgui_draw.cpp \
	$(IMGUI_DIR)/imgui_tables.cpp \
	$(IMGUI_DIR)/imgui_widgets.cpp \
	$(IMGUI_BACKENDS_DIR)/imgui_impl_sdl2.cpp \
	$(IMGUI_BACKENDS_DIR)/imgui_impl_opengl3.cpp

# Object files
OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.c=.o)  # Also replace .c with .o

# Detect OS
UNAME_S := $(shell uname -s)

# Compiler flags
CXXFLAGS = -std=c++17 -I$(SRC_DIR) -I$(MUGEN_DIR) -I$(LODEPNG_DIR) -I$(IMGUI_DIR) -I$(IMGUI_BACKENDS_DIR) -I$(GLAD_DIR)

# If the target 'debug' is being built
ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS += -fsanitize=address -g -DDEBUG
else
CXXFLAGS += -O2 -Wall -Wformat
endif

LIBS =

# Platform-specific settings
ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`
	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin)
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib
	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
    ECHO_MESSAGE = "MinGW"
    LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`
    CXXFLAGS += `pkg-config --cflags sdl2`
    CFLAGS = $(CXXFLAGS)
endif

# Build rules
all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

debug: $(EXE)
	@echo Debug build complete for $(ECHO_MESSAGE)

# Embed resource: lua script
$(RES_DIR)/lua_script.o: $(RES_DIR)/main.lua
	ld -r -b binary -o $@ $<

ifeq ($(OS), Windows_NT)
$(RESFILE): $(RCFILE) $(ICON)
	$(RC) $(RCFILE) -o $(RESFILE)
	
$(EXE): $(OBJS) $(RESFILE) $(RES_DIR)/lua_script.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)
else
$(EXE): $(OBJS) $(RES_DIR)/lua_script.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)
endif

# Generic compile rule for .cpp and .c
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(EXE) $(OBJS)

install: $(EXE)
	strip -s $(EXE)
ifeq ($(OS), Windows_NT)
	install -m 755 $(EXE) /f/PortableApps/MugenSpriteViewer-win64/$(EXE)
else
	sudo install -m 755 $(EXE) /usr/bin/$(EXE)
endif