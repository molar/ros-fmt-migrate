# Clang-tidy external module example

This project demonstrates how to write an external (out-of-llvm-source) plugin for clang-tidy, that can be used with the ``--load`` flag for clang-tidy.

## instructions

  mkdir build
  cd build && cmake .. -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1 
  ninja

  clang-tidy-14 --load=$PWD/libMyLint.so --list-checks -checks=*
