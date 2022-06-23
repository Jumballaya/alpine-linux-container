# Directories
DIR_BIN := dist
DIR_SRC := src
DIR_INC := inc

# Files
FILE_INC := $(wildcard src/*.cpp)

# General
CXX := g++
CXXFLAGS := -lcurl
NAME := container
BIN := $(DIR_BIN)/$(NAME)

.PHONY: run
run: $(BIN)
	$(BIN)

$(DIR_BIN):
	mkdir -p dist

$(BIN): $(DIR_BIN)
	$(CXX) $(FILE_INC) $(CXXFLAGS) -o $(BIN)

clean:
	rm -rf root
	rm -rf $(DIR_BIN)

build: clean $(BIN)