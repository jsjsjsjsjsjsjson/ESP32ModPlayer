# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)
# 优化级别
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer -fdata-sections -ffunction-sections -fdiagnostics-show-option -fno-rtti -fno-exceptions -ftree-vectorize -ftree-loop-vectorize -ffunction-sections -fdata-sections -funroll-loops -finline-functions -fassociative-math -fno-math-errno -freciprocal-math -funsafe-math-optimizations -fno-signed-zeros -g0 -Ofast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fdata-sections -ffunction-sections -fdiagnostics-show-option -fno-rtti -fno-exceptions -ftree-vectorize -ftree-loop-vectorize -ffunction-sections -fdata-sections -funroll-loops -finline-functions -fassociative-math -fno-math-errno -freciprocal-math -funsafe-math-optimizations -fno-signed-zeros -g0 -Ofast")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")