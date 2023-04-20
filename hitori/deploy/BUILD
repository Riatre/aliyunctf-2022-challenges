load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "pkg_filegroup", "pkg_attributes")

pkg_zip(
    name = "source_zip",
    srcs = [
        "//:pkg_handout_source",
    ],
    out = "hitori.zip",
)

pkg_files(
    name = "deploy_scripts",
    attributes = pkg_attributes(
        mode = "0755",
    ),
    srcs = [
        "prepare.sh",
        "start.sh",
        "stop.sh",
    ]
)

pkg_files(
    name = "deploy_files",
    srcs = [
        "flag",
        "//:Dockerfile",
        "//:docker-compose.yaml",
    ],
)

pkg_filegroup(
    name = "pkg_deploy",
    srcs = [
        ":deploy_scripts",
        ":deploy_files",
        "//:pkg_deploy_binary",
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
    mode = "0644",
)
