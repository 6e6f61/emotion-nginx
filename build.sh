#!/usr/bin/env sh
set -e

docker compose exec ps2dev sh -c "apk add make gmp-dev mpfr-dev mpc1; cd /project; make"