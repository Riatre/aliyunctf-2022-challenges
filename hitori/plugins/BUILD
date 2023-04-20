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
        ":d",
        ":gaussian_blur",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_b.so",
    srcs = ["filter_b.cc"],
    linkshared = True,
    deps = [
        ":median_filter",
        ":e",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_c.so",
    srcs = ["filter_c.cc"],
    linkshared = True,
    deps = [
        ":laplacian",
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
    name = "gaussian_blur",
    srcs = ["gaussian_blur.cc"],
    hdrs = ["gaussian_blur.h"],
    deps = [
        ":image_utils",
        "@mdspan",
    ],
)

cc_library(
    name = "median_filter",
    srcs = ["median_filter.cc"],
    hdrs = ["median_filter.h"],
    deps = [
        ":image_utils",
        "@mdspan",
    ],
)

cc_library(
    name = "laplacian",
    srcs = ["laplacian.cc"],
    hdrs = ["laplacian.h"],
    deps = [
        ":image_utils",
    ]
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

cc_library(
    name = "image_utils",
    srcs = ["image_utils.cc"],
    hdrs = ["image_utils.h"],
    deps = [
        "//:canvas",
        "@mdspan",
        "@com_google_absl//absl/log:check",
    ],
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
