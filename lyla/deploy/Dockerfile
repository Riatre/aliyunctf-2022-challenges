# redpwn/jail v0.3.1
# See https://github.com/redpwn/jail#competitor-faq
# tl;dr nothing special here, just a glorified xinetd
FROM pwn.red/jail@sha256:cacd8a66d681647ab1b6740b979f55e5788c135d6272f8c8ee5d62e8cfbc167c

# jammy: Ubuntu 22.04
COPY --from=ubuntu:jammy-20221130 / /srv

COPY lyla /srv/app/run
