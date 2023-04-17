cc_binary(
    name = "hitori",
    srcs = ["main.cc"],
    features = ["static_libstdcxx"],
    deps = [
        ":canvas",
        ":plugin",
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
    ],
)

cc_library(
    name = "plugin",
    hdrs = ["plugin.h"],
)
