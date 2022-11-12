project(ExternalClangTidyModule)
cmake_minimum_required(VERSION 3.18)
# Find CMake file for Clang
find_package(Clang REQUIRED CONFIG NO_DEFAULT_PATH PATHS /usr/lib/llvm-15/lib/cmake)
find_package(roscpp REQUIRED)
include_directories(${CLANG_INCLUDE_DIRS})

add_library(MyLint SHARED main.cpp 
  RosstreamtofmtCheck.cpp
  HeaderincludeguardCheck.cpp
  ReorderCtorInitializer.cpp
  MoveConstantInitToDeclaration.cpp
  )
target_link_libraries(MyLint 
  clangTidy
  clangTidyUtils
  clangTooling
  )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(examples examples/ex1.cpp)
target_include_directories(examples PRIVATE ${roscpp_INCLUDE_DIRS})
target_link_libraries(examples ${roscpp_LIBRARIES})
target_compile_features(examples PRIVATE cxx_std_17)
set_target_properties(examples PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )
