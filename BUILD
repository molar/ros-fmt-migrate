load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh",
    targets = {
        "//...": "",
    },
)

cc_library(
    name = "llvm_headers",
    hdrs = [
        "@llvm_toolchain_llvm//:all_includes",
    ],
)

cc_binary(
    name = "custom_plugin.so",
    srcs = [
        "HeaderincludeguardCheck.cpp",
        "HeaderincludeguardCheck.h",
        "ReorderCtorInitializer.cpp",
        "ReorderCtorInitializer.h",
        "RosstreamtofmtCheck.cpp",
        "RosstreamtofmtCheck.h",
        "main.cpp",
    ],
    copts = [
        "-Iexternal/llvm_toolchain_llvm/include",
        "-std=c++17",
        "-stdlib=libstdc++",
        "-fno-exceptions",
        "-fno-rtti",
    ],
    linkshared = True,
    linkstatic = False,
    deps = [":llvm_headers"],
)

sh_binary(
    name = "run",
    srcs = ["run.sh"],
    data = [
        ":custom_plugin.so",
        "@llvm_toolchain_llvm//:clang-tidy",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

sh_test(
    name = "test_plugin",
    srcs = ["test.sh"],
    data = [
        "run",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
