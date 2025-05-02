cmake_minimum_required(VERSION 3.20)
project(CdbgExpr LANGUAGES CXX)

# Set C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -Wall -Wextra")
include_directories(include)

# Source files
file(GLOB SRC_FILES src/*.cpp)

# Target library
add_library(CdbgExpr STATIC ${SRC_FILES})