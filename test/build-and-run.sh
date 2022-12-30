#!/bin/bash
set -euo pipefail

gcc test.c ../src/jsonc.c -I. -I../src -Wextra -Wall -Werror -Wconversion -ggdb -o testsuite
./testsuite
