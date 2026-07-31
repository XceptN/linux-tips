If you want to inspect a container image without downloading, you can use `skopeo copy`.
e.g. `skopeo copy docker://source-registry.com/image:tag docker://dest-registry.com/image:tag`