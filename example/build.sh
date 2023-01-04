#!/bin/bash
set -euo pipefail

gcc main.c ../src/jsonc.c ../src/string_builder.c ../src/olh_map.c -I. -I../src -Wextra -Wall -Werror -Wconversion -ggdb -O2 -o jpp
