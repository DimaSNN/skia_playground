cmake_minimum_required(VERSION 3.10)
project(SkiaPlayground)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

# set(SKIA_DIR "/home/ashimgaev/git/skia")
set(SKIA_DIR "./skia")

# Find the Skia library
find_library(SKIA_LIBRARY skia PATHS ${SKIA_DIR}/out/Shared)

# Include Skia headers
include_directories(${SKIA_DIR}/)
# include_directories(${SKIA_DIR}/include/core)
# include_directories(${SKIA_DIR}/include/effects)
# include_directories(${SKIA_DIR}/out/Shared)


# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})



# Add Skia include directories
# include_directories(${skia_SOURCE_DIR})
# include_directories(${skia_BINARY_DIR})

# Add your source files
add_executable(SkiaPlayground main_2.cpp spark.cpp point.cpp contour_transformator.cpp)
target_include_directories(SkiaPlayground PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_BUILD_TYPE Debug)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link Skia libraries
# target_link_libraries(SkiaPlayground ${SKIA_LIBRARY})
target_link_libraries(SkiaPlayground ${SKIA_LIBRARY} ${SDL2_LIBRARIES} Threads::Threads)
