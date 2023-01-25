#!/bin/bash -e

./prepare.sh
docker-compose up --force-recreate --no-build -d
