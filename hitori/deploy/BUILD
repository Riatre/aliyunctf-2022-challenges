load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

pkg_zip(
    name = "source_zip",
    srcs = [
        "//:pkg_source_handout",
    ],
    out = "hitori.zip",
)

pkg_files(
    name = "pkg_deploy",
    srcs = [
        "flag",
        "prepare.sh",
        "start.sh",
        "stop.sh",
        "//:Dockerfile",
        "//:docker-compose.yaml",
    ],
    prefix = "题目部署文档及相关文件/",
)

pkg_tar(
    name = "release",
    srcs = [
        ":pkg_deploy",
        "//docs",
    ],
    extension = ".tar.gz",
    files = {
        "//:handout": "参赛选手下载文件/handout.tar.gz",
        ":source_zip": "题目相关源码/hitori.zip",
    },
)
