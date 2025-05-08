cmake_minimum_required(VERSION 3.20)

# Set C++ standard and compiler flags
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SRC_FILES src/*.cpp)

# Target library
add_library(CdbgExpr STATIC ${SRC_FILES})

target_include_directories(CdbgExpr PUBLIC
    include
)