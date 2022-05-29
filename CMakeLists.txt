project(ExternalClangTidyModule)
cmake_minimum_required(VERSION 3.18)
set(CXX_STANDARD 17)
# Find CMake file for Clang
find_package(Clang REQUIRED CONFIG NO_DEFAULT_PATH PATHS /usr/lib/llvm-14/lib/cmake)

include_directories(${CLANG_INCLUDE_DIRS})

add_library(MyLint SHARED main.cpp RosstreamtofmtCheck.cpp)
target_link_libraries(MyLint 
  clangTidy
  clangTooling
  )
