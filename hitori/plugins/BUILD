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
        "//:plugin",
        ":a",
        ":d",
    ],
)

cc_binary(
    name = "filter_b.so",
    srcs = ["filter_b.cc"],
    linkshared = True,
    deps = [
        "//:plugin",
        ":b",
        ":e",
    ],
)

cc_binary(
    name = "filter_c.so",
    srcs = ["filter_c.cc"],
    linkshared = True,
    deps = [
        "//:plugin",
        ":c",
        ":f",
    ],
)

cc_binary(
    name = "filter_d.so",
    srcs = ["filter_d.cc"],
    linkshared = True,
    deps = [
        "//:plugin",
        ":d",
        ":e",
        ":f",
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
