load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
package(default_visibility = ["//visibility:public"])

filegroup(
    name = "example_images",
    srcs = glob(["*.png"]),
)

pkg_files(
    name = "pkg_handout_binary",
    srcs = [
        "README.txt",
    ],
    prefix = "assets/",
)

pkg_files(
    name = "pkg_handout_source",
    srcs = [
        "README.txt",
        "BUILD",
    ],
    prefix = "assets/",
)

pkg_files(
    name = "pkg",
    srcs = [
        ":example_images",
    ],
    prefix = "assets/",
)
