#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
opp_makemake -f -e cc -o assignments
make MODE=release -j"$(nproc 2>/dev/null || echo 2)"
make MODE=debug -j"$(nproc 2>/dev/null || echo 2)"
