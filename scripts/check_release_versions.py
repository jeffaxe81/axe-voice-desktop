#!/usr/bin/env python3
"""Check native candidate release metadata using only the Python standard library."""
import argparse
from pathlib import Path
import re
import sys


MARKERS = {
    'CMakeLists.txt': r'\bproject\s*\(\s*AXEVoiceDesktop\s+VERSION\s+([^\s)]+)',
    'installer/axe-native-candidate.iss': r'^\s*AppVersion\s*=\s*(\S+)\s*$',
    'installer/CANDIDATE-README.txt': r'\AAXE Voice Native Candidate ([^\r\n]+)(?:\r?\n|\Z)',
    'src/diagnostic_export.h': r'L"AXE Voice candidate ([^"\\]+)\\n"',
    'src/main.cpp': r'L"AXE Voice \| Candidato nativo ([^"]+)"',
}


def check_versions(root):
    versions = {}
    for name, pattern in MARKERS.items():
        matches = re.findall(pattern, (root / name).read_text(encoding='utf-8'), re.MULTILINE)
        if len(matches) != 1:
            raise ValueError(f'{name}: expected exactly one release version marker, found {len(matches)}')
        version = matches[0]
        if not re.fullmatch(r'\d+\.\d+(?:\.\d+)?', version):
            raise ValueError(f'{name}: invalid release version {version!r}')
        versions[name] = version
    expected = versions['CMakeLists.txt']
    if not re.fullmatch(r'\d+\.\d+\.\d+', expected):
        raise ValueError(f'CMakeLists.txt: expected a three-part release version, got {expected!r}')
    for name, version in versions.items():
        allowed = {expected}
        if name == 'src/main.cpp' and expected.endswith('.0'):
            allowed.add(expected[:-2])
        if version not in allowed:
            raise ValueError(f'{name}: release version {version!r} does not match {expected!r}')
    return expected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', nargs='?', type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    try:
        version = check_versions(args.root)
    except (OSError, UnicodeError, ValueError) as error:
        print(f'Release version check failed: {error}', file=sys.stderr)
        return 1
    print(f'Release versions agree: {version}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
