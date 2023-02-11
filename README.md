# Clang-tidy external module example

This project demonstrates how to write an external (out-of-llvm-source) plugin for clang-tidy, that can be used with the ``--load`` flag for clang-tidy.
depending on how your upstream llvm compiler was built you might need different flags (e.g ``-fnortti``)

## instructions

```
bazel test //...
bazel run //:run --  --checks="-*,mir-*" $PWD/examples/ex1.cpp
```
