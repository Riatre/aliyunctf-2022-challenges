load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")
load("@bazel_skylib//rules:write_file.bzl", "write_file")

exports_files([
    "Dockerfile",
    "docker-compose.yaml",
])

cc_binary(
    name = "hitori",
    srcs = ["main.cc"],
    data = [
        "//assets:example_images",
        "//plugins",
    ],
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
    visibility = ["//visibility:public"],
)

cc_library(
    name = "plugin",
    hdrs = ["plugin.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":canvas",
        "@com_google_absl//absl/status",
    ],
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

pkg_files(
    name = "pkg_binary",
    srcs = [
        ":hitori.stripped",
    ],
    attributes = pkg_attributes(
        mode = "0755",
    ),
    renames = {
        "hitori.stripped": "hitori",
    },
)

pkg_files(
    name = "pkg_source",
    srcs = glob([
        "*.cc",
        "*.h",
    ]) + [
        ".bazelrc",
        ".bazelversion",
        ".clang-format",
        "BUILD",
        "WORKSPACE",
    ],
)

pkg_filegroup(
    name = "pkg_handout_binary",
    srcs = [
        ":pkg_binary",
        "//assets:pkg_handout_binary",
        "//plugins:pkg_binary",
    ],
    prefix = "bin/",
)

pkg_filegroup(
    name = "pkg_deploy_binary",
    srcs = [
        ":pkg_binary",
        "//assets:pkg",
        "//plugins:pkg_binary",
    ],
    prefix = "bin/",
    visibility = ["//visibility:public"],
)

pkg_filegroup(
    name = "pkg_handout_source",
    srcs = [
        ":pkg_source",
        "//bazel:pkg",
        "//assets:pkg_handout_source",
        "//plugins:pkg_source",
    ],
    prefix = "src/",
    visibility = ["//visibility:public"],
)

write_file(
    name = "local_test_flag",
    out = "flag",
    content = ["aliyunctf{real_flag_on_server}"],
)

pkg_tar(
    name = "handout",
    srcs = [
        "Dockerfile",
        "docker-compose.yaml",
        ":local_test_flag",
        ":pkg_handout_binary",
        ":pkg_handout_source",
    ],
    extension = ".tar.gz",
    visibility = ["//visibility:public"],
)
