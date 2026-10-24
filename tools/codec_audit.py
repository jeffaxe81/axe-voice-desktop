#!/usr/bin/env python3
"""Offline AXE Voice/Baresip codec module candidate audit (does not load binaries).

Public, standalone tooling: NOT proof that SIP/SDP/RTP calls or codecs work.
Does not package a codec and does not approve third-party distribution terms.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys
from typing import Any


TARGETS = {"windows-x64": ".dll", "linux-x64": ".so"}
# Identifiers refer to Baresip modules, not to negotiated SDP codec names.
MODULES = {
    "PCMA/PCMU (G.711)": {"module": "g711", "dependency": None, "required": True},
    "G.722": {"module": "g722", "dependency": None, "required": True},
    "Opus": {"module": "opus", "dependency": "opus", "required": True},
    "GSM 06.10": {"module": "gsm", "dependency": "gsm", "required": False},
    "Speex": {"module": "speex", "dependency": "speex", "required": False},
    "G.729": {"module": "g729", "dependency": "g729", "required": False},
}
# Baresip upstream does not guarantee a standalone Speex codec module.
# G.729 must never be unlocked on file presence or an unchecked license claim.
SPECIAL = {"Speex": "requires separately verified compatible codec module",
           "G.729": "license and codec module require external manual review"}


def binary_arch(path: Path) -> str:
    """Recognize PE64 AMD64 or ELF64 x86_64 without executing a candidate."""
    try:
        with path.open("rb") as stream:
            header = stream.read(64)
            if len(header) < 20:
                return "unrecognized"
            if header[:4] == b"\x7fELF":
                if header[4] == 2 and header[5] in (1, 2):
                    machine = int.from_bytes(header[18:20], "little" if header[5] == 1 else "big")
                    return "linux-x64" if machine == 62 else "other-elf"
                return "other-elf"
            if header[:2] == b"MZ" and len(header) >= 64:
                offset = struct.unpack_from("<I", header, 0x3C)[0]
                if offset > 16 * 1024 * 1024:
                    return "unrecognized"
                stream.seek(offset)
                pe = stream.read(6)
                if pe[:4] == b"PE\x00\x00":
                    return "windows-x64" if pe[4:6] == b"\x64\x86" else "other-pe"
            return "unrecognized"
    except (OSError, ValueError):
        return "unreadable"


def direct_files(directory: Path, suffix: str) -> dict[str, Path]:
    """Only scan direct regular files; do not follow symlinks or traverse trees."""
    if not directory.is_dir():
        return {}
    try:
        return {p.name.lower(): p for p in directory.iterdir()
                if not p.is_symlink() and p.is_file() and p.name.lower().endswith(suffix)}
    except OSError:
        return {}


def dependency_aliases(name: str, suffix: str) -> set[str]:
    aliases = {
        "opus": ("opus", "libopus", "opus-0"),
        "gsm": ("gsm", "libgsm"),
        "speex": ("speex", "libspeex", "speex-1"),
        "g729": ("g729", "libg729", "bcg729", "libbcg729"),
    }
    return {f"{part}{suffix}" for part in aliases[name]}


def audit(modules_dir: Path, libraries_dirs: list[Path], target: str) -> dict[str, Any]:
    """Return an evidence inventory. 'candidate' never means callable/compatible."""
    if target not in TARGETS:
        raise ValueError(f"unsupported target: {target}")
    ext = TARGETS[target]
    modules = direct_files(modules_dir, ext)
    libraries: dict[str, Path] = {}
    for directory in libraries_dirs:
        libraries.update(direct_files(directory, ext))
    # Baresip module directories may also have the corresponding dependency DLL.
    for filename, path in modules.items():
        libraries.setdefault(filename, path)
    rows = []
    for name, spec in MODULES.items():
        filename = spec["module"] + ext
        mod = modules.get(filename)
        mod_arch = binary_arch(mod) if mod else None
        depname = spec["dependency"]
        dep = None
        dep_arch = None
        if depname:
            dep = next((libraries[k] for k in sorted(dependency_aliases(depname, ext))
                        if k in libraries and libraries[k] != mod), None)
            dep_arch = binary_arch(dep) if dep else None
        if mod is None:
            state = "module-missing"
        elif mod_arch != target:
            state = "module-architecture-mismatch"
        elif depname and dep is None:
            state = "dependency-missing"
        elif depname and dep_arch != target:
            state = "dependency-architecture-mismatch"
        elif name == "G.729":
            state = "manual-license-review-required"
        elif name == "Speex":
            state = "custom-module-review-required"
        else:
            state = "candidate-only"
        rows.append({
            "codec": name, "module": filename, "required_for_basic_profile": spec["required"],
            "state": state, "module_path": str(mod) if mod else None,
            "module_arch": mod_arch, "dependency_path": str(dep) if dep else None,
            "dependency_arch": dep_arch,
            "note": SPECIAL.get(name, "file/architecture check only; live SIP/SDP/RTP tests required"),
            "homologated": False,
        })
    return {"schema_version": 1, "target": target, "inspection": "offline-no-execution",
            "modules_directory": str(modules_dir),
            "library_directories": [str(x) for x in libraries_dirs],
            "codecs": rows,
            "basic_profile_file_candidates": sum(r["state"] == "candidate-only" for r in rows if r["required_for_basic_profile"]),
            "basic_profile_total": sum(r["required_for_basic_profile"] for r in rows),
            "live_media_verified": False, "commercial_distribution_approved": False}


def candidates_only_config(report: dict[str, Any]) -> str:
    """Emit commented examples; never activate unverified modules."""
    lines = ["# AXE Voice codec candidates (not tested or enabled).",
             "# Review licensing, dependencies, module ABI and SIP/SDP/RTP before enabling."]
    for row in report["codecs"]:
        if row["state"] == "candidate-only":
            lines.append(f"# module {row['module']}  # {row['codec']} [UNVERIFIED]")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modules", type=Path, required=True, help="Baresip module directory")
    parser.add_argument("--libs", type=Path, action="append", default=[], help="External library directory (repeatable)")
    parser.add_argument("--target", choices=sorted(TARGETS), required=True)
    parser.add_argument("--report", type=Path, help="Write JSON evidence report")
    parser.add_argument("--commented-config", type=Path, help="Write a commented-only Baresip module fragment")
    parser.add_argument("--fail-on-missing-basic", action="store_true", help="Return 2 if basic codec files are not candidates")
    args = parser.parse_args(argv)
    report = audit(args.modules, args.libs, args.target)
    rendered = json.dumps(report, indent=2, ensure_ascii=False) + "\n"
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    if args.commented_config:
        args.commented_config.parent.mkdir(parents=True, exist_ok=True)
        args.commented_config.write_text(candidates_only_config(report), encoding="utf-8")
    if args.fail_on_missing_basic and report["basic_profile_file_candidates"] < report["basic_profile_total"]:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
