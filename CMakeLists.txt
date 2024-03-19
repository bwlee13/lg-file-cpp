cmake_minimum_required(VERSION 3.25)
project(LgFile)

set(CMAKE_CXX_STANDARD 20)

#message(STATUS "set compiler")
#set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm")
#set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm")
#set(OPENMP_INCLUDES "/opt/homebrew/Cellar/llvm/17.0.6_1/include")

add_executable(lg_file main.cpp)

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
find_package(TBB CONFIG REQUIRED)
#find_package(OpenMP REQUIRED)

include_directories(${ARROW_INCLUDE_DIR})
include_directories(${PARQUET_INCLUDE_DIR})

#target_compile_options(lg_file PRIVATE ${OpenMP_C_FLAGS})
target_link_libraries(lg_file PRIVATE ${ARROW_SHARED_LIB} ${PARQUET_SHARED_LIB} TBB::tbb)
