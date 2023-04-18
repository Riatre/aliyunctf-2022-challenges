load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "plugins",
    srcs = [
        "filter_a.so",
        "filter_b.so",
        "filter_c.so",
        "filter_d.so",
    ],
)

cc_binary(
    name = "filter_a.so",
    srcs = ["filter_a.cc"],
    linkshared = True,
    deps = [
        ":a",
        ":d",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_b.so",
    srcs = ["filter_b.cc"],
    linkshared = True,
    deps = [
        ":b",
        ":e",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_c.so",
    srcs = ["filter_c.cc"],
    linkshared = True,
    deps = [
        ":c",
        ":f",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_d.so",
    srcs = ["filter_d.cc"],
    linkshared = True,
    deps = [
        ":d",
        ":e",
        ":f",
        "//:plugin",
    ],
)

cc_library(
    name = "a",
    srcs = ["a.cc"],
    hdrs = ["a.h"],
)

cc_library(
    name = "b",
    srcs = ["b.cc"],
    hdrs = ["b.h"],
)

cc_library(
    name = "c",
    srcs = ["c.cc"],
    hdrs = ["c.h"],
)

cc_library(
    name = "d",
    srcs = ["d.cc"],
    hdrs = ["d.h"],
)

cc_library(
    name = "e",
    srcs = ["e.cc"],
    hdrs = ["e.h"],
)

cc_library(
    name = "f",
    srcs = ["f.cc"],
    hdrs = ["f.h"],
)

pkg_files(
    name = "pkg_binary",
    srcs = [":plugins"],
    prefix = "plugins/",
)

pkg_files(
    name = "pkg_source",
    srcs = glob([
        "**/*.h",
        "**/*.cc",
    ]) + ["BUILD"],
    prefix = "plugins/",
)
