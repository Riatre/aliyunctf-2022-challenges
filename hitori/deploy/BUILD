load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")

pkg_zip(
    name = "source_zip",
    srcs = [
        "//:pkg_handout_source",
    ],
    out = "hitori.zip",
)

pkg_files(
    name = "deploy_scripts",
    srcs = [
        "prepare.sh",
        "start.sh",
        "stop.sh",
    ],
    attributes = pkg_attributes(
        mode = "0755",
    ),
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
        ":deploy_files",
        ":deploy_scripts",
        "//:pkg_deploy_binary",
    ],
    prefix = "题目部署文档及相关文件/",
)

pkg_tar(
    name = "deploy_tar",
    srcs = [
        ":pkg_deploy",
    ],
)

genrule(
    name = "image",
    srcs = [":deploy_tar"],
    outs = ["image.tar.zst"],
    cmd = " && ".join([
        "TMP=$$(mktemp -d || mktemp -d -t bazel-tmp)",
        "tar --strip-components 1 -C $$TMP -xf $(location :deploy_tar)",
        "docker buildx build -f $$TMP/Dockerfile --output type=docker,dest=- -t ghcr.io/pizzatql/aliyunctf-hitori:latest $$TMP | zstd -c > $(OUTS)",
    ]),
    local = True,
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
        ":image": "题目部署文档及相关文件/image.tar.zst",
    },
    mode = "0644",
)
