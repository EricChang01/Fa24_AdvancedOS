# Variables
CXX = g++                 
CXXFLAGS = -O0  # Compilation flags

TARGET_APAGER = apager 
TARGET_PROG1 = prog1
TARGET_PROG2 = prog2

LD_APAGER = loader.ld
LD_PROG = default.ld

SRC_LOADER = segment.cpp pager.cpp stack.cpp
HDR_LOADER = segment.hpp pager.hpp stack.hpp
OBJ_LOADER = $(SRC_LOADER:.cpp=.o)

SRC_APAGER = apager.cpp
OBJ_APAGER = apager.o

SRC_PROG1 = prog1.cpp
SRC_PROG2 = prog2.cpp

DEBUG = -D StackBuild

all: $(TARGET_APAGER) $(TARGET_PROG1) $(TARGET_PROG2)

$(TARGET_PROG1): $(SRC_PROG1) $(LD_PROG)
	$(CXX) $(CXXFLAGS) -static -o $(TARGET_PROG1) $(SRC_PROG1) -T $(LD_PROG)

$(TARGET_PROG2): $(SRC_PROG2) $(LD_PROG)
	$(CXX) $(CXXFLAGS) -static -o $(TARGET_PROG2) $(SRC_PROG2) -T $(LD_PROG)

# Rule to compile .cpp files into .o object files
%.o: %.cpp $(HDR_LOADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to compile the loader executable
$(TARGET_APAGER): $(OBJ_APAGER) $(OBJ_LOADER)
	$(CXX) $(CXXFLAGS) -static $(OBJ_LOADER) $(OBJ_APAGER) -o apager

# Clean up generated files
clean:
	rm -f $(TARGET_APAGER) $(OBJ_APAGER) $(TARGET_PROG1) $(OBJ_LOADER) $(TARGET_PROG2)

.PHONY: all clean run
