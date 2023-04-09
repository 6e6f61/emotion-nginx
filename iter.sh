#!/usr/bin/env sh
set -e

./build.sh
/run/wrappers/bin/pcsx2 --elf="emngx.elf"