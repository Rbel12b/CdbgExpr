# Compiler and flags
CXX := g++
CXXFLAGS := -g -fsanitize=address -Wall -Wextra -std=c++23 -Iinclude

# Target library
TARGET := libCdbgExpr.a

# Source files
SRCDIR := ./src
SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(SRC:.cpp=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.cpp .stamp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm .stamp
	rm -f $(OBJ) $(TARGET)

.stamp:
ifeq ($(OS),Windows_NT)
	echo > .stamp
else
	touch .stamp
endif