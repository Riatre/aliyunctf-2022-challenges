load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files")
package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "basic_docs",
    srcs = ["题目基本信息.xlsx"],
)

pkg_files(
    name = "build_docs",
    srcs = ["BUILD.md"],
    prefix = "题目相关源码/",
)

pkg_files(
    name = "deploy_docs",
    srcs = ["deploy.md"],
    prefix = "题目部署文档及相关文件/",
    renames = {
        "deploy.md": "README.md",
    },
)

pkg_files(
    name = "writeup",
    srcs = ["writeup.md"],
    prefix = "题目writeup/",
)

pkg_filegroup(
    name = "docs",
    srcs = [
        ":basic_docs",
        ":build_docs",
        ":deploy_docs",
        ":writeup",
    ],
)
