# redpwn/jail v0.3.1
# See https://github.com/redpwn/jail#competitor-faq
# tl;dr nothing special here, just a glorified xinetd
FROM pwn.red/jail@sha256:cacd8a66d681647ab1b6740b979f55e5788c135d6272f8c8ee5d62e8cfbc167c

COPY --from=ubuntu:jammy-20221130 / /srv
COPY bin/hitori /srv/app/run
COPY bin/assets/ /srv/app/assets/
COPY bin/plugins/ /srv/app/plugins/

ENV JAIL_MEM=128M
ENV JAIL_TMP_SIZE=32M
ENV JAIL_TIME=120
