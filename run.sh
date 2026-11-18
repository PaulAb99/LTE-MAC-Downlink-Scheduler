#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# If invoked by the OMNeT++ IDE with launch arguments (e.g., -m -n . omnetpp.ini),
# forward them directly to ./assignments so Qtenv opens interactively.
if [ "$#" -gt 0 ]; then
    exec ./assignments "$@"
fi

if [ ! -x ./assignments ]; then
    ./makemake.sh
fi

rm -f metrics.csv

echo "=== Running LTE MAC Scheduling Load & Algorithm Sweep ==="
./assignments -u Cmdenv -f omnetpp.ini -c LteLoadSweep

echo "=== Generating Plots & Summary Tables ==="
python3 plot_results.py
