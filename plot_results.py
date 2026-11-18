#!/usr/bin/env python3
"""Generates summary tables and plots from metrics.csv for the LTE MAC Scheduler."""

import csv
import os
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.abspath(__file__))
METRICS = os.path.join(ROOT, "metrics.csv")
OUT = os.path.join(ROOT, "results")

ALGS = ["fixed", "rr", "wrr", "lq", "os", "pf"]
LOADS = [0.3, 0.5, 0.7, 0.9]
CLASSES = ["HP", "MP", "LP"]


def load_csv(path):
    if not os.path.isfile(path):
        return []
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def fnum(row, key, default=0.0):
    try:
        return float(row.get(key) or default)
    except ValueError:
        return default


def inum(row, key, default=0):
    try:
        return int(float(row.get(key) or default))
    except ValueError:
        return default


def mean_weighted(rows, value_key="mean_delay", weight_key="packets"):
    num = 0.0
    den = 0.0
    for row in rows:
        w = fnum(row, weight_key)
        if w <= 0:
            continue
        num += fnum(row, value_key) * w
        den += w
    if den <= 0:
        return None
    return num / den


def print_table(title, headers, data):
    print("\n" + title)
    widths = [len(h) for h in headers]
    str_rows = []
    for row in data:
        cells = [str(c) for c in row]
        str_rows.append(cells)
        for i, c in enumerate(cells):
            widths[i] = max(widths[i], len(c))
    line = "  ".join(h.ljust(widths[i]) for i, h in enumerate(headers))
    print(line)
    print("  ".join("-" * w for w in widths))
    for cells in str_rows:
        print("  ".join(cells[i].ljust(widths[i]) for i in range(len(headers))))


def fmt_ms(seconds):
    if seconds is None:
        return "-"
    return "%.3f" % (seconds * 1000.0)


def main():
    rows = load_csv(METRICS)
    if not rows:
        print("File %s not found. Run ./run.sh first." % METRICS)
        return 1

    os.makedirs(OUT, exist_ok=True)

    # 1. System mean delay vs load (30%, 50%, 70%, 90%)
    lte_sys = [r for r in rows if inum(r, "user") == -1]
    by_alg = defaultdict(list)
    for r in lte_sys:
        by_alg[r["algorithm"]].append((round(fnum(r, "load"), 2), fnum(r, "mean_delay")))

    table = []
    for alg in ALGS:
        points = dict(by_alg.get(alg, []))
        if not points:
            continue
        table.append([alg] + [fmt_ms(points.get(load)) for load in LOADS])
    print_table(
        "LTE System Mean Delay [ms] vs Network Load",
        ["algorithm"] + [str(int(x * 100)) + "%" for x in LOADS],
        table,
    )

    # 2. Per-priority class mean delay (HP, MP, LP) at 70% and 90% load
    for target_load in (0.7, 0.9):
        per_user = [
            r for r in rows
            if inum(r, "user") >= 0
            and abs(fnum(r, "load") - target_load) < 1e-6
        ]
        if not per_user:
            continue
        prio_table = []
        for alg in ALGS:
            alg_rows = [r for r in per_user if r["algorithm"] == alg]
            if not alg_rows:
                continue
            row_cells = [alg]
            for prio in CLASSES:
                subset = [r for r in alg_rows if r["priority"] == prio]
                row_cells.append(fmt_ms(mean_weighted(subset)))
            prio_table.append(row_cells)
        print_table(
            "LTE Mean Delay [ms] by Priority Class at %d%% Load (HP: W=8,6 | MP: W=2 | LP: W=1)"
            % int(target_load * 100),
            ["algorithm", "HP_ms", "MP_ms", "LP_ms"],
            prio_table,
        )

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except Exception as exc:
        print("\nmatplotlib not installed (%s); tables above show the results." % exc)
        return 0

    if lte_sys:
        plt.figure(figsize=(7, 4.5))
        all_y = []
        for alg in ALGS:
            pts = sorted(by_alg.get(alg, []))
            if not pts:
                continue
            ys = [p[1] * 1000 for p in pts]
            all_y.extend(ys)
            plt.plot([p[0] * 100 for p in pts], ys, marker="o", label=alg)
        plt.xlabel("Network Load [%]")
        plt.ylabel("System Mean Packet Delay [ms]")
        plt.title("LTE Downlink MAC Scheduling: Mean Delay vs Network Load")
        if all_y and max(all_y) > 25.0:
            plt.yscale("symlog", linthresh=0.5)
        else:
            plt.ylim(bottom=0)
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(os.path.join(OUT, "lte_delay_vs_load.png"), dpi=150)
        plt.close()

    for target_load, fname in ((0.7, "lte_delay_by_class_70.png"), (0.9, "lte_delay_by_class_90.png")):
        per_user = [
            r for r in rows
            if inum(r, "user") >= 0
            and abs(fnum(r, "load") - target_load) < 1e-6
        ]
        if not per_user:
            continue
        present_algs = [a for a in ALGS if any(r["algorithm"] == a for r in per_user)]
        plt.figure(figsize=(7.5, 4.5))
        x0 = list(range(len(present_algs)))
        width = 0.25
        all_bars = []
        for j, prio in enumerate(CLASSES):
            vals = []
            for alg in present_algs:
                subset = [r for r in per_user if r["algorithm"] == alg and r["priority"] == prio]
                m = mean_weighted(subset)
                vals.append(0 if m is None else m * 1000)
            all_bars.extend(vals)
            plt.bar([x + (j - 1) * width for x in x0], vals, width=width, label=prio)
        plt.xticks(x0, present_algs)
        plt.ylabel("Mean Packet Delay [ms]")
        plt.title("LTE Mean Delay by User Priority Class (%d%% Load)" % int(target_load * 100))
        if all_bars and max(all_bars) > 25.0:
            plt.yscale("symlog", linthresh=0.5)
        else:
            plt.ylim(bottom=0)
        plt.legend()
        plt.grid(True, axis="y", alpha=0.3)
        plt.tight_layout()
        plt.savefig(os.path.join(OUT, fname), dpi=150)
        plt.close()

    print("\nSaved plots to %s" % OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
