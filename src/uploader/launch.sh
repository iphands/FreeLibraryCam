#!/bin/bash
set -e
mkdir -p secrets
cp -r ../../secrets/imgur.sh ./secrets/
docker build -t camserver .
docker run --restart always --rm --name camserver -p 8000:8000 camserver
