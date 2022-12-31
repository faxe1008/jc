#!/bin/bash
set -euo pipefail

gcc test.c ../src/jsonc.c ../src/string_builder.c -I. -I../src -Wextra -Wall -Werror -Wconversion -ggdb -o testsuite
./testsuite
