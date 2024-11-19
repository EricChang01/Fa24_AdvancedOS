# Variables
CXX = g++                 # C++ compiler
CXXFLAGS = -g -Wall        # Compilation flags: -g for debugging, -Wall for all warnings
TARGET_PROGRAM ?= default_program  # Target program to test, default is 'default_program'
SRC = main.cpp             # C++ source file (you can add more .cpp files here)
OBJ = $(SRC:.cpp=.o)       # Object files (generated from .cpp)
DEPS =                     # Dependencies (if any)

# Default target
all: $(TARGET_PROGRAM)

# Rule to build the target program (which is passed as a variable)
$(TARGET_PROGRAM): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET_PROGRAM)

# Rule to compile .cpp files into .o files
%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET_PROGRAM) $(OBJ)

# Run the program
run: $(TARGET_PROGRAM)
	./$(TARGET_PROGRAM)

.PHONY: all clean run
