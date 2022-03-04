# the compiler to use, gcc for c and g++ for c++
CC = g++

# vars:
RM = rm -rf
BUILD_DIR=_build

# compiler flags:
# -g     -for adding debugging information to the executable file
# -wall  -for turning on most compiler warnings
CFLAGS = -g -Wall 

# linker flags:
LFLAGS  = -I./inc

# additional flags:
AFLAGS = -lpthread

# additional src files
CFILES = 

# the target to build
TARGET = main

all: $(TARGET)

$(TARGET): src/$(TARGET).cpp
	[ -d $(BUILD_DIR) ] || mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) src/$(TARGET).cpp $(CFILES) $(LFLAGS) $(AFLAGS)

run:
	@printf " --- Running executable ---\n\n"
	@_build/$(TARGET)
	@printf "\n\n --- DONE ---\n"

clean:
	$(RM) _build
