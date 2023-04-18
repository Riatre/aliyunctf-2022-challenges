load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

exports_files(glob(["*.BUILD"]))
pkg_files(
    name = "pkg",
    srcs = glob(["**"]),
    prefix = "bazel/",
)
