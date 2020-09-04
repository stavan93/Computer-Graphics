CC = gcc
CXX = g++ -std=c++14

INCLUDES = -I$(glad_inc) -I/usr/local/include
LIBRARIES = 

source_dir = .

glad_inc = $(source_dir)/deps

CFLAGS = -Wall -ggdb -O3 $(INCLUDES)
CXXFLAGS = -Wall -ggdb -O3 $(INCLUDES)

LDFLAGS = $(LIBRARIES) -lglfw -lGL -lX11 -lXxf86vm -lXrandr -lpthread -ldl -lXinerama -lXcursor

TARGET = name
cpp_files = name.cpp mesh.cpp shader.cpp window.cpp gl_error.cpp grid.cpp \
            camera.cpp util.cpp flat_lighter.cpp

TARGET2 = uncross
cpp_files2 = uncross.cpp mesh.cpp shader.cpp window.cpp gl_error.cpp grid.cpp \
            camera.cpp util.cpp flat_lighter.cpp

c_files = deps/glad.c

objects = $(cpp_files:.cpp=.o) $(c_files:.c=.o)

objects2 = $(cpp_files2:.cpp=.o) $(c_files:.c=.o)

all: $(TARGET) $(TARGET2)

$(TARGET): $(objects)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TARGET2): $(objects2)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY : clean
clean :
	-rm -f $(TARGET) $(TARGET2) $(objects) $(objects2)

