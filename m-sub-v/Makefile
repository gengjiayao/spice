ifeq ($(SYSTEMC_HOME),)
$(error "SYSTEMC_HOME environment variable not set!")
endif

CXX      := g++
TARGET   := build/exe
SRCS     := main.cpp

CXXFLAGS := -I$(SYSTEMC_HOME)/include -Wall
LDFLAGS  := -L$(SYSTEMC_HOME)/lib-linux64
LDLIBS   := -lsystemc

OBJS     := $(addprefix build/, $(SRCS:.cpp=.o))

$(shell mkdir -p build)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build
