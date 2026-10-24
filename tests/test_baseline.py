"""Offline regression tests for the static, nonfunctional AXE Voice baseline gate."""
import pathlib
import tempfile
import unittest
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "scripts"))
from verify_baseline import BaselineError, BASELINE, MANIFEST, PROVENANCE, SOURCE, parse_manifest, verify


class BaselineGuardTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        source = self.root / SOURCE
        source.mkdir(parents=True)
        (source / "const.h").write_bytes(
            b'#define _GLOBAL_VERSION "3.22.16"\n#define _GLOBAL_VERSION_COMMA 3,22,16,0\n')
        (source / "microsip.vcxproj").write_text("<Project/>", encoding="utf-8")
        manifest = self.root / MANIFEST
        manifest.parent.mkdir(parents=True)
        lines = [f"{BASELINE}/{p.relative_to(source).as_posix()}\t{p.stat().st_size} bytes"
                 for p in sorted(source.rglob("*")) if p.is_file()]
        manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")
        provenance = self.root / PROVENANCE
        provenance.write_text(
            "MicroSIP-3.22.16-src.7z sha256: "
            "9c1ff942ae1ae56bdfcdd04c3d6e242a056d7f0dd50bb19f6e13226e7c819b83",
            encoding="utf-8")
        licenses = self.root / "LICENSES/README.md"
        licenses.parent.mkdir(parents=True)
        licenses.write_text("GNU GPL v2; PJSIP license requires review", encoding="utf-8")

    def check(self):
        return verify(self.root, expected_count=2)

    def test_valid_reference_tree(self):
        result = self.check()
        self.assertEqual(result["imported_files_checked"], 2)
        self.assertIs(result["build_executed"], False)
        self.assertIs(result["sip_calls_tested"], False)
        self.assertIs(result["commercial_distribution_approved"], False)

    def test_reject_traversal(self):
        with self.assertRaises(BaselineError):
            parse_manifest(BASELINE + "/../secrets\t2 bytes\n")

    def test_reject_duplicate(self):
        entry = BASELINE + "/const.h\t3 bytes\n"
        with self.assertRaises(BaselineError):
            parse_manifest(entry + entry)

    def test_reject_absolute_and_backslash(self):
        for name in ("/etc/passwd", BASELINE + "\\const.h"):
            with self.subTest(name=name), self.assertRaises(BaselineError):
                parse_manifest(name + "\t1 bytes\n")

    def test_reject_malformed_size(self):
        with self.assertRaises(BaselineError):
            parse_manifest(BASELINE + "/const.h\tfake bytes")

    def test_missing_file_fails(self):
        (self.root / SOURCE / "const.h").unlink()
        with self.assertRaisesRegex(BaselineError, "missing"):
            self.check()

    def test_extra_file_fails(self):
        (self.root / SOURCE / "unreviewed.cpp").write_text("new", encoding="utf-8")
        with self.assertRaisesRegex(BaselineError, "unexpected"):
            self.check()

    def test_size_change_fails(self):
        (self.root / SOURCE / "microsip.vcxproj").write_text("changed", encoding="utf-8")
        with self.assertRaisesRegex(BaselineError, "size-changed"):
            self.check()

    def test_modified_version_fails(self):
        const = self.root / SOURCE / "const.h"
        original = const.read_bytes()
        const.write_bytes(original.replace(b"3.22.16", b"3.22.17"))
        # A size-preserving change must still be rejected by semantic version verification.
        with self.assertRaisesRegex(BaselineError, "version"):
            self.check()

    def test_missing_provenance_fails(self):
        (self.root / PROVENANCE).write_text("incorrect SHA", encoding="utf-8")
        with self.assertRaisesRegex(BaselineError, "provenance"):
            self.check()

    def test_missing_license_inventory_fails(self):
        (self.root / "LICENSES/README.md").write_text("none", encoding="utf-8")
        with self.assertRaisesRegex(BaselineError, "license"):
            self.check()

    def test_excluded_ide_user_config_is_explicit(self):
        manifest = self.root / MANIFEST
        with manifest.open("a", encoding="utf-8") as handle:
            handle.write(f"{BASELINE}/microsip.vcxproj.user\\t165 bytes\\n")
        report = verify(self.root, expected_count=None)
        self.assertEqual(report["imported_files_checked"], 2)
        self.assertEqual(report["imported_files_in_manifest"], 3)
        self.assertEqual(report["excluded_ide_local_files"], ["microsip.vcxproj.user"])

    def test_ide_exclusion_size_must_match_original(self):
        manifest = self.root / MANIFEST
        with manifest.open("a", encoding="utf-8") as handle:
            handle.write(f"{BASELINE}/microsip.vcxproj.user\\t999 bytes\\n")
        with self.assertRaisesRegex(BaselineError, "missing"):
            verify(self.root, expected_count=None)

    def test_empty_manifest_rejected(self):
        with self.assertRaises(BaselineError):
            parse_manifest("")

    def test_expected_inventory_size_checked(self):
        with self.assertRaisesRegex(BaselineError, "number"):
            verify(self.root)


if __name__ == "__main__":
    unittest.main()
