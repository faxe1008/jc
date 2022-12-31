#!/bin/bash
set -euo pipefail

gcc main.c ../src/jsonc.c ../src/string_builder.c -I. -I../src -Wextra -Wall -Werror -Wconversion -ggdb -o jpp