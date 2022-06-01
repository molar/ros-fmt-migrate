# Clang-tidy external module example

This project demonstrates how to write an external (out-of-llvm-source) plugin for clang-tidy, that can be used with the ``--load`` flag for clang-tidy.

## instructions

    # deps for the example
    sudo apt install ros-base
    # cmake
    mkdir build
    cd build && cmake .. -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1 
    ninja

    # back in the main dir
    clang-tidy-14 --load=$PWD/build/libMyLint.so -checks=mir-rosstreamfmt  -header-filter=.\* -system-headers examples/ex1.cpp

