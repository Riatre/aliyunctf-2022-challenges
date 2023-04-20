load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "plugins",
    srcs = [
        "cloudy_clear.so",
        "cutting_edge.so",
        "extremely_accessible.so",
        "mtxx.so",
    ],
)

cc_binary(
    name = "cloudy_clear.so",
    srcs = ["cloudy_clear.cc"],
    linkshared = True,
    deps = [
        ":histogram_equalization",
        ":median_filter",
        "//:plugin",
    ],
)

cc_binary(
    name = "cutting_edge.so",
    srcs = ["cutting_edge.cc"],
    linkshared = True,
    deps = [
        ":automagical_edge_detector",
        ":color_converter",
        "//:plugin",
    ],
)

cc_binary(
    name = "extremely_accessible.so",
    srcs = ["extremely_accessible.cc"],
    linkshared = True,
    deps = [
        ":automagical_edge_detector",
        ":enhancer",
        ":median_filter",
        "//:plugin",
    ],
)

cc_binary(
    name = "mtxx.so",
    srcs = ["mtxx.cc"],
    linkshared = True,
    deps = [
        ":enhancer",
        "//:plugin",
    ],
)

cc_library(
    name = "automagical_edge_detector",
    srcs = ["automagical_edge_detector.cc"],
    hdrs = ["automagical_edge_detector.h"],
    deps = [
        ":gaussian_blur",
        ":image_utils",
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
    name = "color_converter",
    srcs = ["color_converter.cc"],
    hdrs = ["color_converter.h"],
    deps = [
        ":image_utils",
    ],
)

cc_library(
    name = "image_utils",
    srcs = ["image_utils.cc"],
    hdrs = ["image_utils.h"],
    deps = [
        "//:canvas",
        "@com_google_absl//absl/log:check",
        "@mdspan",
    ],
)

cc_library(
    name = "histogram_equalization",
    srcs = ["histogram_equalization.cc"],
    hdrs = ["histogram_equalization.h"],
    deps = [
        ":image_utils",
    ],
)

cc_library(
    name = "enhancer",
    srcs = ["enhancer.cc"],
    hdrs = ["enhancer.h"],
    deps = [
        ":image_utils",
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
