"""Execute 1,000 real native SIP registration attempts against isolated loopback fixtures.

Requires the native SDK smoke binary (NOT the UI-only AXE-Voice.exe).
This is NOT validation against a production PABX. No credentials leave localhost.

Usage:
  python tests/run_1000_native_sip_connections.py path/to/native_sip_engine_smoke.exe
  python tests/run_1000_native_sip_connections.py path/to/native_sip_engine_smoke.exe --count 1000 --report sip-results.json
"""
import argparse
import json
import pathlib
import random
import sys
import time

from run_native_sip_loopback import scenario


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=pathlib.Path)
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=20261106)
    parser.add_argument("--report", type=pathlib.Path, default=pathlib.Path("sip-connection-results.json"))
    args = parser.parse_args()
    if not args.executable.is_file():
        parser.error("Native SIP smoke executable not found; build with AXV_ENABLE_NATIVE_SIP first")
    if not 1 <= args.count <= 100000:
        parser.error("count must be 1..100000")
    modes = ("success", "wrong-password", "timeout", "missing-audio")
    cases = [modes[i % len(modes)] for i in range(args.count)]
    random.Random(args.seed).shuffle(cases)
    results = []
    try:
        for index, mode in enumerate(cases, 1):
            began = time.monotonic()
            try:
                scenario(str(args.executable.resolve()), mode)
                status, error = "passed", None
            except Exception as exc:
                status, error = "failed", f"{type(exc).__name__}: {exc}"
            results.append({"case": index, "mode": mode, "status": status,
                            "duration_seconds": round(time.monotonic() - began, 3),
                            "error": error})
            if index % 25 == 0 or status == "failed":
                print(f"[{index}/{args.count}] passed={sum(r['status']=='passed' for r in results)} "
                      f"failed={sum(r['status']=='failed' for r in results)}", flush=True)
    except KeyboardInterrupt:
        print("Interrupted; writing partial report", file=sys.stderr)
    finally:
        summary = {"requested": args.count, "executed": len(results),
                   "passed": sum(r["status"] == "passed" for r in results),
                   "failed": sum(r["status"] == "failed" for r in results),
                   "environment": "localhost mock SIP registrar; not an external PBX",
                   "cases": results}
        args.report.write_text(json.dumps(summary, indent=2), encoding="utf-8")
        print(f"Results: {args.report} ({summary['executed']} executed, {summary['failed']} failed)")
    return 0 if len(results) == args.count and not summary["failed"] else 1


if __name__ == "__main__":
    sys.exit(main())
