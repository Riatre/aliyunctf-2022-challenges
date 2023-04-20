load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

package(default_visibility = ["//visibility:public"])

# A: Median Filter -> Histogram equalization
# B: Grayscale -> Gaussian -> Auto-detect Edge Detector (Sobel, Laplacian or Canny; vtable) [DONE]
# C: Increase Saturation -> Brighten
# D: Median Filter -> Brighten(image, Gaussian on first channel only -> auto-detect edge detector)

filegroup(
    name = "plugins",
    srcs = [
        "cloudy_clear.so",
        "cutting_edge.so",
        "filter_c.so",
        "filter_d.so",
    ],
)

cc_binary(
    name = "cloudy_clear.so",
    srcs = ["cloudy_clear.cc"],
    linkshared = True,
    deps = [
        ":median_filter",
        ":histogram_equalization",
        "//:plugin",
    ],
)

cc_binary(
    name = "cutting_edge.so",
    srcs = ["cutting_edge.cc"],
    linkshared = True,
    deps = [
        ":color_converter",
        ":automagical_edge_detector",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_c.so",
    srcs = ["filter_c.cc"],
    linkshared = True,
    deps = [
        ":f",
        ":laplacian",
        "//:plugin",
    ],
)

cc_binary(
    name = "filter_d.so",
    srcs = ["filter_d.cc"],
    linkshared = True,
    deps = [
        ":color_converter",
        ":sobel",
        ":f",
        "//:plugin",
    ],
)

cc_library(
    name = "automagical_edge_detector",
    srcs = ["automagical_edge_detector.cc"],
    hdrs = ["automagical_edge_detector.h"],
    deps = [
        ":image_utils",
        ":gaussian_blur",
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
