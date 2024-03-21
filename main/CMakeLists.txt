idf_component_register(SRCS "keypad.c" "main.c"
                    INCLUDE_DIRS ".")


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer -fdata-sections -ffunction-sections -fdiagnostics-show-option -fno-rtti -fno-exceptions -ftree-vectorize -ftree-loop-vectorize -ffunction-sections -fdata-sections -funroll-loops -finline-functions -fassociative-math -fno-math-errno -freciprocal-math -funsafe-math-optimizations -fno-signed-zeros -g0 -Ofast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fdata-sections -ffunction-sections -fdiagnostics-show-option -fno-rtti -fno-exceptions -ftree-vectorize -ftree-loop-vectorize -ffunction-sections -fdata-sections -funroll-loops -finline-functions -fassociative-math -fno-math-errno -freciprocal-math -funsafe-math-optimizations -fno-signed-zeros -g0 -Ofast")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")