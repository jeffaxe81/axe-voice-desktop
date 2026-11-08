#!/usr/bin/env python3
"""Fail-closed, same-SHA engineering delivery certificate.

This validator reads only GitHub Actions metadata and artifacts. It DOES NOT
start calls, access SIP credentials or contact any PABX. A green Windows build
is not enough: the native SDK, 1000 local SIP tests, both installers, WinUI3,
upstream integrity and security gates must all have the same main commit SHA.
"""
import argparse
import json
import os
from pathlib import Path
import re
import sys
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

RUN_NAMES = {
    "windows": "AXE Voice Windows CI",
    "baseline": "AXE Voice | integridade da baseline",
    "winui": "AXE Voice WinUI3 Preview",
    "native": "AXE Voice native SDK smoke",
}
EXPECTED_ARTIFACTS = {
    "windows": ("AXE-Voice-Windows-UI-Only-No-SIP", "AXE-Voice-Policy-10M-Results"),
    "baseline": ("axe-voice-static-baseline-{sha}",),
    "winui": ("AXE-Voice-WinUI3-Preview-No-SIP",),
    "native": (
        "AXE-Voice-Native-SIP-1000-Results",
        "AXE-Voice-Native-Candidate-Setup",
        "AXE-Voice-WinUI3-Native-SIP-Lab-Candidate",
        "AXE-Voice-WinUI3-Native-Lab-Setup",
    ),
}
EXPECTED_GATES = (
    "windows_build_and_ctest", "upstream_integrity",
    "native_sdk_abi_and_reactor", "sip_loopback_1000",
    "native_installer_acceptance", "win32_compact_ui",
    "winui3_lab_candidate", "credentials_and_security_checks",
)


def successful_steps(jobs, job_name_fragment, *steps):
    matches = [job for job in jobs if job_name_fragment in job.get("name", "")
               and job.get("status") == "completed" and job.get("conclusion") == "success"]
    if len(matches) != 1:
        return False
    by_name = {step.get("name"): step for step in matches[0].get("steps", [])}
    return all(by_name.get(name, {}).get("status") == "completed" and
               by_name[name].get("conclusion") == "success" for name in steps)


def verify_evidence(sha, runs, jobs_by_id, artifacts_by_id, native_run_id=None):
    """Pure function; conservative defaults and no production approval."""
    if not re.fullmatch(r"[0-9a-f]{40}", sha):
        raise ValueError("Expected full 40-character Git commit SHA")
    selected, issues = {}, []
    for kind, name in RUN_NAMES.items():
        options = [r for r in runs if r.get("name") == name and
                   r.get("head_sha") == sha and r.get("head_branch") == "main" and
                   r.get("event") == "push"]
        if kind == "native" and native_run_id is not None:
            options = [r for r in options if r.get("id") == native_run_id]
        if not options:
            issues.append(f"Missing exact-main-SHA {name} run")
            continue
        run = max(options, key=lambda x: x.get("id", 0))
        selected[kind] = run
        if run.get("status") != "completed" or run.get("conclusion") != "success":
            issues.append(f"{name} did not complete successfully for {sha[:12]}")
    def jobs(kind):
        run = selected.get(kind)
        return jobs_by_id.get(run["id"], []) if run else []
    def artifacts_ok(kind):
        run = selected.get(kind)
        if not run or run.get("status") != "completed" or run.get("conclusion") != "success":
            return False
        artifacts = artifacts_by_id.get(run["id"], [])
        available = {a.get("name") for a in artifacts if not a.get("expired") and
                     isinstance(a.get("size_in_bytes"), int) and a["size_in_bytes"] > 0}
        required = {name.format(sha=sha) for name in EXPECTED_ARTIFACTS[kind]}
        return required.issubset(available)
    def passed(kind):
        r = selected.get(kind, {})
        return r.get("status") == "completed" and r.get("conclusion") == "success"

    windows_build = passed("windows") and successful_steps(
        jobs("windows"), "build-and-test", "Build and test with project script",
        "Run 10M deterministic policy scenarios (no live PBX)",
        "Publish 10M scenario report",
        "Verify quality gates (unit, blackbox, integration)")
    baseline = passed("baseline") and artifacts_ok("baseline") and successful_steps(
        jobs("baseline"), "Integridade MicroSIP",
        "Executar testes de integridade (não testes SIP)",
        "Validar aceite de engenharia sem homologacao PBX real",
        "Conferir 231 arquivos da importação original")
    sdk = passed("native") and successful_steps(
        jobs("native"), "msvc-native-sdk (abi)",
        "Build pinned MSVC candidate and run real libre reactor") and successful_steps(
        jobs("native"), "msvc-native-sdk (tls-media)",
        "Build pinned MSVC candidate and run real libre reactor")
    loopback = sdk and successful_steps(
        jobs("native"), "msvc-native-sdk (tls-media)",
        "Execute 1000 native Baresip SIP loopback connection tests",
        "Publish evidence for 1000 SIP connection tests") and artifacts_ok("native")
    native_installer = sdk and successful_steps(
        jobs("native"), "msvc-native-sdk (tls-media)",
        "Build and verify candidate installer",
        "Publish candidate installer") and artifacts_ok("native")
    winui = passed("winui") and artifacts_ok("winui") and successful_steps(
        jobs("winui"), "build-winui3",
        "Test five WinUI 3 preview feature policies (TDD)",
        "Restore and compile real WinUI 3 project",
        "Publish unpackaged UI-only preview with native bridge")
    winui_lab = winui and sdk and successful_steps(
        jobs("native"), "msvc-native-sdk (tls-media)",
        "Package WinUI 3 with actual native SIP host (laboratory only)",
        "Build and verify second Windows installer (WinUI 3 native LAB)",
        "Publish WinUI 3 native LAB installer") and artifacts_ok("native")
    gates = {
        "windows_build_and_ctest": windows_build and artifacts_ok("windows"),
        "upstream_integrity": baseline,
        "native_sdk_abi_and_reactor": sdk,
        "sip_loopback_1000": loopback,
        "native_installer_acceptance": native_installer,
        "win32_compact_ui": windows_build and artifacts_ok("windows"),
        "winui3_lab_candidate": winui_lab,
        "credentials_and_security_checks": windows_build and baseline and sdk and
            successful_steps(jobs("native"), "msvc-native-sdk (tls-media)",
                             "Verify literal package file integrity"),
    }
    for name, passed_gate in gates.items():
        if not passed_gate:
            issues.append(f"Engineering gate missing/failed: {name}")
    return {
        "schema_version": 1, "sha": sha, "branch": "main",
        "engineering_delivery": "approved" if all(gates.values()) else "blocked",
        "gates": gates, "completed": sum(gates.values()), "total": len(gates),
        "evidence_runs": {kind: {"id": r["id"], "name": r["name"],
                                "url": r.get("html_url")}
                          for kind, r in selected.items()},
        "external_live_telephony": "deferred_not_homologated",
        "production_approved": False,
        "issues": issues,
    }


def github_get(repo, route, token):
    url = f"https://api.github.com/repos/{repo}/{route}"
    request = Request(url, headers={
        "Authorization": "Bearer " + token,
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "AXE-Voice-engineering-gate",
    })
    with urlopen(request, timeout=25) as response:
        return json.load(response)


def check_scope(root):
    scope = json.loads((root / "config/release-scope.json").read_text(encoding="utf-8"))
    if (scope.get("engineering_gates") != list(EXPECTED_GATES) or
            scope.get("engineering_status") != "evidence_required_per_commit" or
            scope.get("real_telephony", {}).get("status") != "deferred_not_homologated" or
            scope.get("real_telephony", {}).get("required_for_production") is not True or
            scope.get("real_telephony", {}).get("required_for_engineering_delivery") is not False or
            scope.get("production", {}).get("approved") is not False):
        raise ValueError("Release policy changed; refusing engineering certificate")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True, help="owner/repo")
    parser.add_argument("--sha", required=True, help="full main commit SHA")
    parser.add_argument("--native-run-id", required=True, type=int)
    parser.add_argument("--output", type=Path, default=Path("evidence/engineering-release.json"))
    args = parser.parse_args()
    # Public GitHub actions metadata only; no PBX access.
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repository):
        parser.error("repository must have owner/repo syntax")
    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        parser.error("GITHUB_TOKEN is required for read-only Actions metadata")
    try:
        check_scope(Path(__file__).resolve().parents[1])
        query = urlencode({"branch": "main", "head_sha": args.sha, "per_page": 100})
        response = github_get(args.repository, "actions/runs?" + query, token)
        runs = response.get("workflow_runs", [])
        jobs_by_id, artifacts_by_id = {}, {}
        ids = {r["id"] for r in runs if r.get("name") in RUN_NAMES.values() and
               r.get("head_sha") == args.sha and r.get("head_branch") == "main"}
        for run_id in ids:
            jobs_by_id[run_id] = github_get(
                args.repository, f"actions/runs/{run_id}/jobs?per_page=100", token
            ).get("jobs", [])
            artifacts_by_id[run_id] = github_get(
                args.repository, f"actions/runs/{run_id}/artifacts?per_page=100", token
            ).get("artifacts", [])
        report = verify_evidence(args.sha, runs, jobs_by_id, artifacts_by_id,
                                 args.native_run_id)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                               encoding="utf-8")
        print(f"Engineering release: {report['completed']}/{report['total']} exact-SHA gates; "
              f"production NOT approved; report={args.output}")
        for issue in report["issues"]:
            print("Gate issue: " + issue, file=sys.stderr)
        return 0 if report["engineering_delivery"] == "approved" else 1
    except (ValueError, KeyError, HTTPError, URLError, OSError) as exc:
        print(f"Engineering release verification failed closed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
