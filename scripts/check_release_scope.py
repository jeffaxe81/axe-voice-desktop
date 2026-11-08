#!/usr/bin/env python3
"""Validate engineering-delivery policy without ever probing a real SIP server."""
import argparse
import json
from pathlib import Path
import re
import sys

ENGINEERING_GATES = [
    "windows_build_and_ctest", "upstream_integrity",
    "native_sdk_abi_and_reactor", "sip_loopback_1000",
    "native_installer_acceptance", "win32_compact_ui",
    "winui3_lab_candidate", "credentials_and_security_checks",
]


def validate(root: Path) -> dict:
    scope_path = root / "config/release-scope.json"
    policy_path = root / "src/release_scope_policy.h"
    cmake_path = root / "CMakeLists.txt"
    scope = json.loads(scope_path.read_text(encoding="utf-8"))
    policy = policy_path.read_text(encoding="utf-8")
    cmake = cmake_path.read_text(encoding="utf-8")
    if not isinstance(scope, dict) or scope.get("schema_version") != 1:
        raise ValueError("release scope schema must be v1")
    version = re.search(r"project\s*\(\s*AXEVoiceDesktop\s+VERSION\s+(\d+\.\d+\.\d+)", cmake)
    if version is None or scope.get("release") != version.group(1):
        raise ValueError("release scope and CMake versions must match")
    if scope.get("profile") != "software_engineering_delivery":
        raise ValueError("unexpected software engineering profile")
    if scope.get("engineering_gates") != ENGINEERING_GATES or scope.get("engineering_status") != "evidence_required_per_commit":
        raise ValueError("engineering gates changed without code review")
    external = scope.get("real_telephony")
    if not isinstance(external, dict) or external.get("status") != "deferred_not_homologated":
        raise ValueError("external acceptance must remain deferred, NOT approved")
    if external.get("required_for_engineering_delivery") is not False or external.get("required_for_production") is not True:
        raise ValueError("external SIP may be deferred only for engineering")
    if not isinstance(external.get("checks"), list) or len(external["checks"]) < 4:
        raise ValueError("real PBX acceptance checks missing")
    prod = scope.get("production")
    if not isinstance(prod, dict) or prod.get("approved") is not False:
        raise ValueError("production release must be blocked without live PBX acceptance")
    for required in ("requires_physical_windows_and_headset", "requires_licensing_review_and_signature", "requires_live_telephony_acceptance"):
        if prod.get(required) is not True:
            raise ValueError(f"production prerequisite missing: {required}")
    if scope.get("excluded_future_features") != [234, 235, 236, 237, 238]:
        raise ValueError("unapproved future features included in release")
    if not re.search(r"kRequireRealTelephonyForEngineeringDelivery\s*=\s*false\s*;", policy):
        raise ValueError("C++ engineering policy conflicts with JSON")
    if not re.search(r"kRequireRealTelephonyForProduction\s*=\s*true\s*;", policy):
        raise ValueError("C++ production policy conflicts with JSON")
    for gate in ENGINEERING_GATES:
        if not re.search(r"\b" + re.escape(gate) + r"\b", policy):
            raise ValueError(f"C++ evidence field missing: {gate}")
    if "PRODUCAO: BLOQUEADA" not in policy or "NAO APROVADOS" not in policy:
        raise ValueError("release diagnostic no longer warns about missing live acceptance")
    return {
        "schema_version": 1, "release": scope["release"],
        "engineering_delivery": "pending_exact_commit_evidence",
        "external_live_telephony": "deferred_not_homologated",
        "production_approved": False,
        "engineering_gates": ENGINEERING_GATES
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    try:
        report = validate(args.root)
        if args.output is not None:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except (OSError, UnicodeError, ValueError, TypeError, KeyError, json.JSONDecodeError) as error:
        print(f"Release scope validation FAILED: {error}", file=sys.stderr)
        return 1
    print(f"Release scope PASS: software engineering profile {report['release']}; "
          "external telephony deferred; production NOT approved")
    return 0


if __name__ == "__main__":
    sys.exit(main())
