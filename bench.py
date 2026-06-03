"""
Boot-time benchmark for Nautilus under coreboot/QEMU.
Runs QEMU N times, captures the microsecond timestamp printed by threaded_init,
and reports aggregate statistics.
"""

import subprocess
import re
import statistics
import sys
import os
import csv

def run_once(cmd, script_dir, pattern):
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        cwd=script_dir
    )
    try:
        for line in proc.stdout:
            print(line, end="", flush=True)
            m = pattern.search(line)
            if m:
                return int(m.group(1))
    finally:
        proc.kill()
        proc.wait()
    return None

def main():
    RUNS = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

    QEMU_CMD = [
        "qemu-system-x86_64",
        "-bios", os.path.join(SCRIPT_DIR, "../coreboot/build/coreboot.rom"),
        "-M", "q35",
        "-m", "2G",
        "-smp", "1",
        "-serial", "stdio",
        "-no-reboot",
        "-display", "none"
    ]

    PATTERN = re.compile(r"time from nautilus entry to before yield:\s+(\d+)\s+us")

    times = []
    for i in range(RUNS):
        print(f"\n--- Run {i+1}/{RUNS} ---")
        t = run_once(QEMU_CMD, SCRIPT_DIR, PATTERN)
        if t is None:
            print("WARNING: boot time line not found, skipping run")
            continue
        times.append(t)
        print(f"  => {t} us")

    if not times:
        print("No successful runs.")
        sys.exit(1)

    csv_path = os.path.join(SCRIPT_DIR, "bench_results.csv")
    write_header = not os.path.exists(csv_path)
    with open(csv_path, "a", newline="") as f:
        w = csv.writer(f)
        if write_header:
            w.writerow(["time_us"])
        for _, t in enumerate(times, 1):
            w.writerow([t])
    print(f"\nRaw results written to {csv_path}")

    print(f"\n=== Results over {len(times)} run(s) ===")
    print(f"  min:    {min(times)} us")
    print(f"  max:    {max(times)} us")
    print(f"  mean:   {statistics.mean(times):.1f} us")

    if len(times) > 1:
        print(f"  median: {statistics.median(times):.1f} us")

if __name__ == '__main__':
    main()
