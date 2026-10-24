#!/usr/bin/env python3
"""Validate the imported MicroSIP reference baseline without compiling or dialing."""
import argparse
import json
import pathlib
import re
from datetime import datetime, timezone

BASELINE = "MicroSIP-3.22.16-src"
SOURCE = pathlib.Path("upstream/microsip-3.22.16.0") / BASELINE
MANIFEST = pathlib.Path("docs/upstream/MICROSIP_3.22.16.0_MANIFEST.txt")
PROVENANCE = pathlib.Path("docs/upstream/MICROSIP_3.22.16.0_PROVENANCE.md")
EXPECTED_FILES = 231
# IDE-specific local settings intentionally excluded by .gitignore during import.
EXCLUDED_LOCAL_SETTINGS = {"microsip.vcxproj.user": 165}
EXPECTED_SHA256 = "9c1ff942ae1ae56bdfcdd04c3d6e242a056d7f0dd50bb19f6e13226e7c819b83"


class BaselineError(ValueError):
    """A required file or invariant is missing or inconsistent."""


def parse_manifest(content):
    """Parse GitHub-imported paths and sizes; reject traversal and duplicate entries."""
    entries = {}
    for line in content.splitlines():
        match = re.fullmatch(r"([^\t\r\n]+)\t([0-9]+) bytes", line)
        if match is None:
            raise BaselineError("Malformed manifest line")
        raw_name, byte_count = match.groups()
        path = pathlib.PurePosixPath(raw_name)
        if (path.is_absolute() or raw_name.startswith(("/", ".")) or
                "\\" in raw_name or any(p in ("", ".", "..") for p in raw_name.split("/")) or
                path.parts[0] != BASELINE or len(path.parts) < 2):
            raise BaselineError("Unsafe manifest path")
        if raw_name in entries:
            raise BaselineError("Duplicate manifest path")
        entries[raw_name] = int(byte_count)
    if not entries:
        raise BaselineError("Empty baseline manifest")
    return entries


def verify(root, expected_count=EXPECTED_FILES):
    """Match every baseline file against the original imported byte-size manifest."""
    root = pathlib.Path(root).resolve()
    entries = parse_manifest((root / MANIFEST).read_text(encoding="utf-8"))
    if expected_count is not None and len(entries) != expected_count:
        raise BaselineError("Unexpected number of imported files")
    expected = {pathlib.PurePosixPath(name).relative_to(BASELINE).as_posix(): size
                for name, size in entries.items()}
    actual = {f.relative_to(root / SOURCE).as_posix(): f.stat().st_size
              for f in (root / SOURCE).rglob("*") if f.is_file() and not f.is_symlink()}
    if len(entries) == EXPECTED_FILES and any(
            expected.get(name) != size for name, size in EXCLUDED_LOCAL_SETTINGS.items()):
        raise BaselineError("The import manifest lacks the documented IDE-only exclusion")
    exclusions = {name for name, size in EXCLUDED_LOCAL_SETTINGS.items()
                  if expected.get(name) == size and name not in actual}
    missing = set(expected) - set(actual) - exclusions
    unexpected = set(actual) - set(expected)
    changed = {name for name in set(expected) & set(actual) if expected[name] != actual[name]}
    if missing or unexpected or changed:
        raise BaselineError("Baseline mismatch: %d missing, %d unexpected, %d size-changed" %
                            (len(missing), len(unexpected), len(changed)))
    if "const.h" not in expected or "microsip.vcxproj" not in expected:
        raise BaselineError("Expected source/project files missing")
    const = (root / SOURCE / "const.h").read_bytes()
    if b'#define _GLOBAL_VERSION "3.22.16"' not in const or (
            b"#define _GLOBAL_VERSION_COMMA 3,22,16,0" not in const):
        raise BaselineError("MicroSIP source version differs from selected baseline")
    provenance = (root / PROVENANCE).read_text(encoding="utf-8")
    if EXPECTED_SHA256 not in provenance or "MicroSIP-3.22.16-src.7z" not in provenance:
        raise BaselineError("Official archive provenance is incomplete")
    license_text = (root / "LICENSES/README.md").read_text(encoding="utf-8")
    if "GNU GPL v2" not in license_text or "PJSIP" not in license_text:
        raise BaselineError("Baseline license and PJSIP limitations not documented")
    return {"status": "static_baseline_verified",
            "imported_files_in_manifest": len(entries),
            "imported_files_checked": len(actual),
            "excluded_ide_local_files": sorted(exclusions),
            "archive_sha256_documented": EXPECTED_SHA256,
            "build_executed": False, "sip_calls_tested": False,
            "commercial_distribution_approved": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=pathlib.Path)
    opts = parser.parse_args()
    report = verify(opts.root)
    report["verified_at"] = datetime.now(timezone.utc).replace(microsecond=0).isoformat()
    if opts.output:
        opts.output.parent.mkdir(parents=True, exist_ok=True)
        opts.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("PASS: %d tracked baseline files checked (%d manifest entries; %d IDE-local exclusions); no build, SIP call or commercialization verified" %
          (report["imported_files_checked"], report["imported_files_in_manifest"],
           len(report["excluded_ide_local_files"])))


if __name__ == "__main__":
    main()
