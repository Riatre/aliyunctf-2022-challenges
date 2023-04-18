cc_binary(
    name = "hitori",
    srcs = ["main.cc"],
    data = glob(["assets/*.png"]) + ["//plugins"],
    features = ["static_libstdcxx"],
    deps = [
        ":canvas",
        ":plugin_manager",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@fmt",
    ],
)

cc_library(
    name = "canvas",
    srcs = ["canvas.cc"],
    hdrs = ["canvas.h"],
    deps = [
        "@com_github_randy408_libspng//:spng",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@fmt",
    ],
)

cc_library(
    name = "plugin",
    hdrs = ["plugin.h"],
    deps = [
        ":canvas",
        "@com_google_absl//absl/status",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "plugin_manager",
    srcs = ["plugin_manager.cc"],
    hdrs = ["plugin_manager.h"],
    linkopts = ["-ldl"],
    deps = [
        ":plugin",
        "@com_google_absl//absl/status:statusor",
    ],
)
