#!/bin/bash
set -euo pipefail

gcc test.c ../src/jsonc.c -I. -I../src -Wextra -Wall -Werror -ggdb -o testsuite
./testsuite
