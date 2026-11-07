"""Run a deterministic, parallel 1,000-case Baresip SIP registration acceptance matrix.

The target executable is the real AXV native SDK smoke program, not the UI-only
binary. All SIP traffic stays on 127.0.0.1; this is NOT a PBX production test.
The four modes are success, wrong-password, timeout, and missing-audio.

Usage:
  python tests/run_1000_native_sip_connections.py build/native-sdk-tls-media/product/Release/axv_native_sip_engine_smoke.exe
  python tests/run_1000_native_sip_connections.py path/to/axv_native_sip_engine_smoke.exe --count 1000 --workers 12 --report build/native-sip-1000.json
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
from pathlib import Path
import random
import sys
import time

from run_native_sip_loopback import scenario


MODES = ("success", "wrong-password", "timeout", "missing-audio")


def run_case(executable, index, mode):
    started = time.monotonic()
    try:
        scenario(executable, mode)
        status, error = "passed", None
    except Exception as exc:
        status, error = "failed", f"{type(exc).__name__}: {exc}"
    return {
        "case": index,
        "mode": mode,
        "status": status,
        "duration_seconds": round(time.monotonic() - started, 3),
        "error": error,
    }


def execute(executable, count, workers, seed):
    cases = [MODES[i % len(MODES)] for i in range(count)]
    random.Random(seed).shuffle(cases)
    results = [None] * count
    passed = failed = completed = 0
    with ThreadPoolExecutor(max_workers=workers, thread_name_prefix="sip-loopback") as pool:
        futures = {
            pool.submit(run_case, executable, i + 1, mode): i
            for i, mode in enumerate(cases)
        }
        for future in as_completed(futures):
            index = futures[future]
            try:
                entry = future.result()
            except Exception as exc:  # Keep a full failure report even on harness bugs.
                entry = {"case": index + 1, "mode": cases[index], "status": "failed",
                         "duration_seconds": None, "error": f"Harness: {type(exc).__name__}: {exc}"}
            results[index] = entry
            completed += 1
            if entry["status"] == "passed":
                passed += 1
            else:
                failed += 1
            if completed % 25 == 0 or completed == count:
                print(f"[{completed}/{count}] passed={passed} failed={failed}", flush=True)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--workers", type=int, default=12)
    parser.add_argument("--seed", type=int, default=20261106)
    parser.add_argument("--report", type=Path, default=Path("sip-connection-results.json"))
    args = parser.parse_args()
    if not args.executable.is_file():
        parser.error("Native SDK smoke executable not found; build the TLS/media native SDK target first")
    if not 1 <= args.count <= 100000:
        parser.error("count must be in 1..100000")
    if not 1 <= args.workers <= 32:
        parser.error("workers must be in 1..32")
    executable = str(args.executable.resolve())
    results = execute(executable, args.count, args.workers, args.seed)
    summary = {
        "requested": args.count,
        "executed": len(results),
        "passed": sum(x["status"] == "passed" for x in results),
        "failed": sum(x["status"] == "failed" for x in results),
        "mode_counts": {mode: {"passed": sum(x["mode"] == mode and x["status"] == "passed" for x in results),
                                "failed": sum(x["mode"] == mode and x["status"] == "failed" for x in results)}
                        for mode in MODES},
        "environment": "127.0.0.1 loopback SIP Digest registrar, native Baresip SDK; no production PBX",
        "cases": results,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Results: {args.report} ({summary['executed']} executed, {summary['passed']} passed, {summary['failed']} failed)")
    return 0 if summary["executed"] == args.count and summary["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
