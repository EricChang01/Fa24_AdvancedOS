# Variables
CXX = g++                 
CXXFLAGS = -static  # Compilation flags
TARGET_APAGER = apager  # Target program to test, default is 'default_program'
TARGET_PROG1 = prog1
SRC_APAGER = apager.cpp       # C++ source file (you can add more .cpp files here)
SRC_PROG1 = prog1.cpp
LD_APAGER = loader.ld
LD_PROG1 = default.ld

DEBUG = -D StackBuild

all: $(TARGET_APAGER) $(TARGET_PROG1)

$(TARGET_APAGER): $(SRC_APAGER) $(LD_APAGER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_APAGER) $(SRC_APAGER) -T $(LD_APAGER) $(DEBUG)

$(TARGET_PROG1): $(SRC_PROG1) $(LD_PROG1)
	$(CXX) $(CXXFLAGS) -o $(TARGET_PROG1) $(SRC_PROG1)  -T $(LD_PROG1)

# Clean up generated files
clean:
	rm -f $(TARGET_APAGER) $(TARGET_PROG1)

# Run the program
run: $(TARGET_APAGER)
	./$(TARGET_APAGER)

.PHONY: all clean run
