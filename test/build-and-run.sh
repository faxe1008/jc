#!/bin/bash
set -euo pipefail

gcc test.c ../src/jsonc.c ../src/string_builder.c ../src/olh_map.c -I. -I../src -Wextra -Wall -Werror -Wconversion -ggdb -o testsuite
./testsuite
