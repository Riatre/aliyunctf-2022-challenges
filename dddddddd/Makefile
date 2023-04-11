IMAGE_NAME=ghcr.io/riatre/aliyunctf-dddddddd
IMAGE_TAG=latest

image: deploy/image.tar.zst

deploy/image.tar.zst: Dockerfile.deploy bin/d8 bin/snapshot_blob.bin
	docker buildx build -f Dockerfile.deploy --output type=docker,dest=- -t $(IMAGE_NAME):$(IMAGE_TAG) . | zstd -c > deploy/image.tar.zst
